const util = require("util");
const cw = require("@aws-sdk/client-cloudwatch");
const nativeStats = require("./nativeStats");

function start() {
  const cwClient = new cw.CloudWatchClient({ region: process.env.AWS_REGION });
  const log = util.debuglog("monitor");
  const debug = util.debuglog("monitor-data");

  let METRICS_INTERVAL =
    parseInt(process.env.METRICS_INTERVAL_OVERRIDE, 10) || 20000; // 20 seconds

  // Set a minimum of 10 seconds
  if (METRICS_INTERVAL < 10000) {
    METRICS_INTERVAL = 10000;
  }

  nativeStats.start();

  // every METRICS_INTERVAL seconds, submit a metrics payload to metricsURL.
  setInterval(() => {
    let {
      ticks,
      gcCount,
      gcTime,
      oldGcCount,
      oldGcTime,
      youngGcCount,
      youngGcTime
    } = nativeStats.sense();
    let totalEventLoopTime = ticks.reduce((a, b) => a + b, 0);
    const ticksData = {};

    ticks.forEach((tick) => {
      if (tick > 0) {
        ticksData[tick] = (ticksData[tick] || 0) + 1;
      }
    });

    const metricData = {
      MetricData: [
        {
          MetricName: "GcCollections",
          Value: gcCount
        },
        {
          MetricName: "GcPause",
          Value: gcTime / 1000,
          Unit: "Microseconds"
        },
        {
          MetricName: "GcOldCollections",
          Value: oldGcCount
        },
        {
          MetricName: "GcOldPause",
          Value: oldGcTime / 1000,
          Unit: "Microseconds"
        },
        {
          MetricName: "GcYoungCollections",
          Value: youngGcCount
        },
        {
          MetricName: "GcYoungPause",
          Value: youngGcTime / 1000,
          Unit: "Microseconds"
        },
        {
          MetricName: "EventLoopUsage",
          Value: totalEventLoopTime / METRICS_INTERVAL * 100,
          Unit: "Percent"
        },
        {
          MetricName: "EventLoopDelay",
          Values: Object.entries(ticksData).map(([key]) => key),
          Counts: Object.entries(ticksData).map(([_, value]) => value),
          Unit: "Milliseconds"
        }
      ],
      Namespace: process.env.AWS_METRICS_NAMESPACE
    }
    debug("[monitor-nodejs-plugin] ", metricData);
    const command = new cw.PutMetricDataCommand(metricData);

    cwClient.send(command, (err) => {
      if (err !== null) {
        log("[monitor-nodejs-plugin] error when trying to submit data: ", err);
      }
    });
  }, METRICS_INTERVAL).unref();
}

module.exports = start;
