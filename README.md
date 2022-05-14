# cloudwatch-nodejs-plugin

This plugin is a modification of [heroku nodejs plugin](https://github.com/heroku/heroku-nodejs-plugin).
Its purpose is to send NodeJS monitoring data such as event loop delays, event loop usage, GC metrics to AWS CloudWatch.

# How does it work?

You can see most of the implementation details in `src/nativeStats.cc`. The plugin sets callbacks
around GC invocations, and during the `prepare` and `check` phases of the event loop, tracks the
amount of time spent in each.

This data is exposed to a JS loop that periodically sends data to AWS CloudWatch.

## Debugging

If the plugin is not working for you once, the first thing
you should do is set the ENV var `NODE_DEBUG` to `monitor`. By default all logging from the plugin
is silenced. If you want to see the actual values sent to CloudWatch, set `NODE_DEBUG` to `monitor-debug`

```
$ NODE_DEBUG=monitor,monitor-debug node -r cloudwatch-nodejs-plugin myapp.js
```

## Metrics collected

```json
{
  GcCollections: 748,
  GcPause: 92179.835,
  GcOldCollections: 2,
  GcOldPause: 671.054,
  GcYoungCollections: 746,
  GcYoungPause: 91508781,
  EventLoopUsage: 12%,
  EventLoopDelay: [array of delays]
}
```

## Development

You can collect and print out metrics locally by running in debug mode.

You can run develop this locally by running build:

```
$ npm run build
```

and including the built module in another local Node app like:

```
$ NODE_OPTIONS="--require {{ working directory }}/cloudwatch-nodejs-plugin/cloudwatch-nodejs-plugin" AWS_METRICS_NAMESPACE="MyApp" node src/index.js
```

Example app with periodic event loop and gc activity: https://github.com/heroku/node-metrics-single-process

## Usage
Require the plugin directly or via `NODE_OPTIONS` variable. `AWS_METRICS_NAMESPACE` should be set. You can optionally set `METRICS_INTERVAL_OVERRIDE` to control the frequency of snapshots.
