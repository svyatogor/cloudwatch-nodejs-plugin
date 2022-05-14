const util = require('util');
const log = util.debuglog('monitor');

// if the node version does not match the major version, bail
const match = process.version.match(/^v([0-9]+)/);
const isExpectedNodeVersion = match && match[1] === NODE_MAJOR_VERSION;

if (isExpectedNodeVersion && process.env.AWS_METRICS_NAMESPACE) {
  log('[monitor-nodejs-plugin] starting');
  const start = require('./monitor.js');
  start();
} else {
  if (!process.env.AWS_METRICS_NAMESPACE) {
    log('[monitor-nodejs-plugin] AWS_METRICS_NAMESPACE not set');
  }
  if (!isExpectedNodeVersion) {
    log("[monitor-nodejs-plugin] expected different Node version. Expected:",
    NODE_MAJOR_VERSION,
    "Found:",
    match && match[1]);
  }
}
