/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

namespace Thunder {
namespace Plugin {

    // Single page viewer served on GET / by the OpenTelemetryCollector. It polls /data/<signal>
    // and renders the OTLP/JSON payloads as tables.
    static constexpr const char ViewerPage[] = R"viewer(<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Thunder OpenTelemetry Collector</title>
<style>
  body { font-family: sans-serif; margin: 0; font-size: 13px; }
  header { background: #263238; color: #fff; padding: 8px 12px; display: flex; gap: 12px; align-items: center; flex-wrap: wrap; }
  header h1 { font-size: 15px; margin: 0 12px 0 0; }
  button { cursor: pointer; }
  .tab { background: #455a64; color: #fff; border: 0; padding: 5px 12px; border-radius: 3px; }
  .tab.active { background: #00897b; }
  #stats { margin-left: auto; font-size: 12px; opacity: 0.85; }
  main { padding: 8px 12px; }
  table { border-collapse: collapse; width: 100%; }
  th, td { border-bottom: 1px solid #ddd; padding: 3px 6px; text-align: left; vertical-align: top; }
  th { background: #eceff1; position: sticky; top: 0; }
  td.num { text-align: right; font-variant-numeric: tabular-nums; white-space: nowrap; }
  td.id { font-family: monospace; white-space: nowrap; }
  td.attrs { font-family: monospace; font-size: 12px; color: #444; }
  tr:hover { background: #f5f5f5; }
  pre { background: #f5f5f5; padding: 8px; overflow: auto; }
  .sev-ERROR, .sev-FATAL { color: #c62828; font-weight: bold; }
  .sev-WARN { color: #ef6c00; }
  #note { color: #666; margin: 4px 0 8px 0; }
</style>
</head>
<body>
<header>
  <h1>Thunder OpenTelemetry Collector</h1>
  <button class="tab" data-tab="traces">Traces</button>
  <button class="tab" data-tab="logs">Logs</button>
  <button class="tab" data-tab="metrics">Metrics</button>
  <input id="filter" placeholder="filter..." size="24">
  <label><input type="checkbox" id="auto" checked> auto refresh</label>
  <button id="refresh">Refresh</button>
  <button id="clear">Clear</button>
  <span id="stats"></span>
</header>
<main>
  <div id="note"></div>
  <div id="content"></div>
</main>
<script>
"use strict";
const $ = (id) => document.getElementById(id);
let tab = "traces";
let lastData = [];

const KINDS = { 0: "UNSPECIFIED", 1: "INTERNAL", 2: "SERVER", 3: "CLIENT", 4: "PRODUCER", 5: "CONSUMER" };

function attrValue(v) {
  if (v === undefined || v === null) return "";
  if ("stringValue" in v) return v.stringValue;
  if ("intValue" in v) return String(v.intValue);
  if ("doubleValue" in v) return String(v.doubleValue);
  if ("boolValue" in v) return String(v.boolValue);
  if ("bytesValue" in v) return String(v.bytesValue);
  if ("arrayValue" in v) return "[" + ((v.arrayValue && v.arrayValue.values) || []).map(attrValue).join(", ") + "]";
  if ("kvlistValue" in v) return "{" + attrs(v.kvlistValue && v.kvlistValue.values) + "}";
  return JSON.stringify(v);
}
function attrs(list) { return (list || []).map((a) => a.key + "=" + attrValue(a.value)).join(", "); }
function service(resource) {
  const a = ((resource && resource.attributes) || []).find((x) => x.key === "service.name");
  return a ? attrValue(a.value) : "";
}
function nanos(v) { try { return BigInt(String(v === undefined ? 0 : v)); } catch (e) { return 0n; } }
function clock(v) {
  const n = nanos(v);
  if (n === 0n) return "";
  const d = new Date(Number(n / 1000000n));
  const p = (x, w) => String(x).padStart(w, "0");
  return p(d.getHours(), 2) + ":" + p(d.getMinutes(), 2) + ":" + p(d.getSeconds(), 2) + "." +
         p(d.getMilliseconds(), 3) + p(Number((n / 1000n) % 1000n), 3);
}
function hue(id) { let h = 0; for (const c of String(id || "")) h = (h * 31 + c.charCodeAt(0)) % 360; return h; }

function cell(row, text, cls) {
  const td = document.createElement("td");
  td.textContent = text === undefined ? "" : text;
  if (cls) td.className = cls;
  row.appendChild(td);
  return td;
}
function table(headers, rows) {
  const filter = $("filter").value.toLowerCase();
  const t = document.createElement("table");
  const head = t.insertRow();
  headers.forEach((h) => { const th = document.createElement("th"); th.textContent = h; head.appendChild(th); });
  let shown = 0;
  rows.forEach((build) => {
    const tr = document.createElement("tr");
    build(tr);
    if ((filter === "") || tr.textContent.toLowerCase().includes(filter)) { t.appendChild(tr); shown++; }
  });
  $("note").textContent = shown + " of " + rows.length + " shown (newest first)";
  return t;
}

function renderTraces(payloads) {
  const spans = [];
  payloads.forEach((p) => (p.resourceSpans || []).forEach((rs) => {
    const svc = service(rs.resource);
    (rs.scopeSpans || []).forEach((ss) => (ss.spans || []).forEach((s) => spans.push({ svc, s })));
  }));
  spans.sort((a, b) => (nanos(b.s.startTimeUnixNano) > nanos(a.s.startTimeUnixNano) ? 1 : -1));
  return table(["Start", "Service", "Name", "Kind", "Duration (us)", "Trace", "Span", "Parent", "Status", "Attributes"],
    spans.map(({ svc, s }) => (tr) => {
      const us = Number(nanos(s.endTimeUnixNano) - nanos(s.startTimeUnixNano)) / 1000;
      cell(tr, clock(s.startTimeUnixNano), "num");
      cell(tr, svc);
      cell(tr, s.name);
      cell(tr, KINDS[s.kind] || s.kind || "");
      cell(tr, us.toFixed(1), "num");
      cell(tr, s.traceId, "id").style.color = "hsl(" + hue(s.traceId) + ",70%,35%)";
      cell(tr, s.spanId, "id");
      cell(tr, s.parentSpanId, "id");
      cell(tr, s.status && (s.status.code !== undefined) ? String(s.status.code) : "");
      cell(tr, attrs(s.attributes), "attrs");
    }));
}

function renderLogs(payloads) {
  const records = [];
  payloads.forEach((p) => (p.resourceLogs || []).forEach((rl) => {
    const svc = service(rl.resource);
    (rl.scopeLogs || []).forEach((sl) => (sl.logRecords || []).forEach((r) => records.push({ svc, r })));
  }));
  const when = (r) => nanos(r.timeUnixNano || r.observedTimeUnixNano);
  records.sort((a, b) => (when(b.r) > when(a.r) ? 1 : -1));
  return table(["Time", "Service", "Severity", "Body", "Trace", "Span", "Attributes"],
    records.map(({ svc, r }) => (tr) => {
      const severity = r.severityText || (r.severityNumber !== undefined ? String(r.severityNumber) : "");
      cell(tr, clock(r.timeUnixNano || r.observedTimeUnixNano), "num");
      cell(tr, svc);
      cell(tr, severity, "sev-" + String(severity).toUpperCase());
      cell(tr, attrValue(r.body));
      cell(tr, r.traceId, "id").style.color = "hsl(" + hue(r.traceId) + ",70%,35%)";
      cell(tr, r.spanId, "id");
      cell(tr, attrs(r.attributes), "attrs");
    }));
}

function renderMetrics(payloads) {
  const pre = document.createElement("pre");
  pre.textContent = payloads.slice().reverse().map((p) => JSON.stringify(p, null, 2)).join("\n\n");
  $("note").textContent = payloads.length + " payload(s), newest first";
  return pre;
}

function render() {
  const content = $("content");
  content.textContent = "";
  content.appendChild(tab === "traces" ? renderTraces(lastData) : (tab === "logs" ? renderLogs(lastData) : renderMetrics(lastData)));
}

async function refresh() {
  try {
    const [data, stats] = await Promise.all([
      fetch("/data/" + tab).then((r) => r.json()),
      fetch("/stats").then((r) => r.json())]);
    lastData = data;
    $("stats").textContent = ["traces", "logs", "metrics"].map((k) =>
      k + ": " + stats[k].requests + " req / " + stats[k].items + " items").join("  |  ");
    render();
  } catch (e) {
    $("note").textContent = "Refresh failed: " + e;
  }
}

function select(name) {
  tab = name;
  document.querySelectorAll(".tab").forEach((b) => b.classList.toggle("active", b.dataset.tab === name));
  refresh();
}

document.querySelectorAll(".tab").forEach((b) => b.addEventListener("click", () => select(b.dataset.tab)));
$("refresh").addEventListener("click", refresh);
$("filter").addEventListener("input", render);
$("clear").addEventListener("click", () => fetch("/clear", { method: "POST" }).then(refresh));
setInterval(() => { if ($("auto").checked) refresh(); }, 2000);
select("traces");
</script>
</body>
</html>
)viewer";

} // namespace Plugin
} // namespace Thunder
