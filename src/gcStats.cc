#include <nan.h>

using namespace v8;

struct HeapInfo {
	size_t totalHeapSize;
	size_t totalHeapExecutableSize;
	size_t totalPhysicalSize;
	size_t usedHeapSize;
	size_t heapSizeLimit;
	size_t totalAvailableSize;
	size_t mallocedMemory;
	size_t peakMallocedMemory;
};

struct HeapData {
	HeapInfo*  before;
	HeapInfo*  after;
	uint64_t   gcStartTime;
	uint64_t   gcEndTime;
	int        gctype;
};

static Nan::Callback* afterGCCallback;

static HeapStatistics beforeGCStats;
uint64_t gcStartTime;

static NAN_GC_CALLBACK(recordBeforeGC) {
	gcStartTime = uv_hrtime();
	Nan::GetHeapStatistics(&beforeGCStats);
}

static void copyHeapStats(HeapStatistics* stats, HeapInfo* info) {
	info->totalHeapSize = stats->total_heap_size();
	info->totalHeapExecutableSize = stats->total_heap_size_executable();
	info->usedHeapSize = stats->used_heap_size();
	info->heapSizeLimit = stats->heap_size_limit();
	info->totalPhysicalSize = stats->total_physical_size();
	info->totalAvailableSize = stats->total_available_size();
	info->mallocedMemory = stats->malloced_memory();
	info->peakMallocedMemory = stats->peak_malloced_memory();
}

static void formatStats(Local<Object> obj, HeapInfo* info) {
	Nan::Set(obj, Nan::New("totalHeapSize").ToLocalChecked(), Nan::New<Number>(info->totalHeapSize));
	Nan::Set(obj, Nan::New("totalHeapExecutableSize").ToLocalChecked(), Nan::New<Number>(info->totalHeapExecutableSize));
	Nan::Set(obj, Nan::New("usedHeapSize").ToLocalChecked(), Nan::New<Number>(info->usedHeapSize));
	Nan::Set(obj, Nan::New("heapSizeLimit").ToLocalChecked(), Nan::New<Number>(info->heapSizeLimit));
	Nan::Set(obj, Nan::New("totalPhysicalSize").ToLocalChecked(), Nan::New<Number>(info->totalPhysicalSize));
	Nan::Set(obj, Nan::New("totalAvailableSize").ToLocalChecked(), Nan::New<Number>(info->totalAvailableSize));
	Nan::Set(obj, Nan::New("mallocedMemory").ToLocalChecked(), Nan::New<Number>(info->mallocedMemory));
	Nan::Set(obj, Nan::New("peakMallocedMemory").ToLocalChecked(), Nan::New<Number>(info->peakMallocedMemory));
}

static void formatStatDiff(Local<Object> obj, HeapInfo* before, HeapInfo* after) {
	Nan::Set(obj, Nan::New("totalHeapSize").ToLocalChecked(), Nan::New<Number>(
		static_cast<double>(after->totalHeapSize) - static_cast<double>(before->totalHeapSize)));
	Nan::Set(obj, Nan::New("totalHeapExecutableSize").ToLocalChecked(), Nan::New<Number>(
		static_cast<double>(after->totalHeapExecutableSize) - static_cast<double>(before->totalHeapExecutableSize)));
	Nan::Set(obj, Nan::New("usedHeapSize").ToLocalChecked(), Nan::New<Number>(
		static_cast<double>(after->usedHeapSize) - static_cast<double>(before->usedHeapSize)));
	Nan::Set(obj, Nan::New("heapSizeLimit").ToLocalChecked(), Nan::New<Number>(
		static_cast<double>(after->heapSizeLimit) - static_cast<double>(before->heapSizeLimit)));
	Nan::Set(obj, Nan::New("totalPhysicalSize").ToLocalChecked(), Nan::New<Number>(
		static_cast<double>(after->totalPhysicalSize) - static_cast<double>(before->totalPhysicalSize)));
	Nan::Set(obj, Nan::New("totalAvailableSize").ToLocalChecked(), Nan::New<Number>(
		static_cast<double>(after->totalAvailableSize) - static_cast<double>(before->totalAvailableSize)));
	Nan::Set(obj, Nan::New("mallocedMemory").ToLocalChecked(), Nan::New<Number>(
		static_cast<double>(after->mallocedMemory) - static_cast<double>(before->mallocedMemory)));
	Nan::Set(obj, Nan::New("peakMallocedMemory").ToLocalChecked(), Nan::New<Number>(
		static_cast<double>(after->peakMallocedMemory) - static_cast<double>(before->peakMallocedMemory)));
}

static void closeCB(uv_handle_t *handle) {
	delete handle;
}

static void asyncCB(uv_async_t *handle) {
	Nan::HandleScope scope;

	HeapData* data = static_cast<HeapData*>(handle->data);

	Local<Object> obj = Nan::New<Object>();
	Local<Object> beforeGCStats = Nan::New<Object>();
	Local<Object> afterGCStats = Nan::New<Object>();

	formatStats(beforeGCStats, data->before);
	formatStats(afterGCStats, data->after);

	Local<Object> diffStats = Nan::New<Object>();
	formatStatDiff(diffStats, data->before, data->after);

	Nan::Set(obj, Nan::New("startTime").ToLocalChecked(),
		Nan::New<Number>(static_cast<double>(data->gcStartTime)));
	Nan::Set(obj, Nan::New("endTime").ToLocalChecked(),
		Nan::New<Number>(static_cast<double>(data->gcEndTime)));
	Nan::Set(obj, Nan::New("pause").ToLocalChecked(),
		Nan::New<Number>(static_cast<double>(data->gcEndTime - data->gcStartTime)));
	Nan::Set(obj, Nan::New("pauseMS").ToLocalChecked(),
		Nan::New<Number>(static_cast<double>((data->gcEndTime - data->gcStartTime) / 1000000)));
	Nan::Set(obj, Nan::New("gctype").ToLocalChecked(), Nan::New<Number>(data->gctype));
	Nan::Set(obj, Nan::New("before").ToLocalChecked(), beforeGCStats);
	Nan::Set(obj, Nan::New("after").ToLocalChecked(), afterGCStats);
	Nan::Set(obj, Nan::New("diff").ToLocalChecked(), diffStats);

	Local<Value> arguments[] = {obj};

	afterGCCallback->Call(1, arguments);

	delete data->before;
	delete data->after;
	delete data;

	uv_close((uv_handle_t*) handle, closeCB);
}

NAN_GC_CALLBACK(afterGC) {
	uv_async_t *async = new uv_async_t;

	HeapData* data = new HeapData;
	data->before = new HeapInfo;
	data->after = new HeapInfo;
	data->gctype = type;

	HeapStatistics stats;

	Nan::GetHeapStatistics(&stats);

	data->gcEndTime = uv_hrtime();
	data->gcStartTime = gcStartTime;

	copyHeapStats(&beforeGCStats, data->before);
	copyHeapStats(&stats, data->after);

	async->data = data;

	uv_async_init(uv_default_loop(), async, asyncCB);
	uv_async_send(async);
}

static NAN_METHOD(AfterGC) {
	if(info.Length() != 1 || !info[0]->IsFunction()) {
		return Nan::ThrowError("Callback is required");
	}

	Local<Function> callbackHandle = info[0].As<Function>();
	afterGCCallback = new Nan::Callback(callbackHandle);

	Nan::AddGCEpilogueCallback(afterGC);
}

NAN_MODULE_INIT(init) {
	Nan::HandleScope scope;
	Nan::AddGCPrologueCallback(recordBeforeGC);

	Nan::Set(target,
		Nan::New("afterGC").ToLocalChecked(),
		Nan::GetFunction(
			Nan::New<FunctionTemplate>(AfterGC)).ToLocalChecked());
}

NODE_MODULE(gcstats, init)
