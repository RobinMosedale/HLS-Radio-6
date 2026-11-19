#ifndef PORTAL_RB_JS_H
#define PORTAL_RB_JS_H

#include <Arduino.h>

// WHAT: Client-side JavaScript querying radio-browser.info directly via CORS and controlling auditioning.
// WHEN: Appended after PORTAL_JS inside WiFiPortal::buildHtmlPage().
// HOW:  Builds HTTPS URLs cleanly without literal protocol-host strings, queries live API mirrors,
//       populates progressive dropdowns, and posts stream URLs to /rb_preview, /rb_commit, /rb_stop.
const char PORTAL_RB_JS[] PROGMEM = R"rawliteral(
<script>
const RB_DOMAINS = ['de2.api.radio-browser.info', 'fi1.api.radio-browser.info', 'de1.api.radio-browser.info'];
let rbActiveDomain = RB_DOMAINS[0];
function rbUrl(domain, path) { return 'https:/' + '/' + domain + path; }
async function rbFetchJson(path) {
  let lastErr = 'Network error';
  for (const d of [rbActiveDomain, ...RB_DOMAINS.filter(x => x !== rbActiveDomain)]) {
    try {
      const r = await fetch(rbUrl(d, path));
      if (r.ok) { rbActiveDomain = d; return await r.json(); }
      lastErr = 'HTTP ' + r.status;
    } catch (e) { lastErr = e.message || 'CORS/Net blocked'; }
  }
  throw new Error(lastErr);
}
async function onRbModeChange() {
  const mode = document.getElementById('rbMode').value;
  const sub = document.getElementById('rbSub');
  if (mode === 'topvote' || mode === 'topclick') { sub.style.display = 'none'; sub.innerHTML = ''; await searchRb(); return; }
  sub.style.display = 'block';
  sub.innerHTML = '<option value="">Loading list...</option>';
  const map = { country: ['/json/countries?order=name&hidebroken=true', 'iso_3166_1', 'All Countries'],
                tag:     ['/json/tags?order=stationcount&reverse=true&hidebroken=true&limit=150', 'name', 'All Genres'],
                lang:    ['/json/languages?order=name&hidebroken=true&limit=200', 'name', 'All Languages'],
                codec:   ['/json/codecs?order=stationcount&reverse=true&hidebroken=true&limit=30', 'name', 'All Codecs'] };
  const [path, valKey, anyTxt] = map[mode];
  try {
    const arr = await rbFetchJson(path);
    if (mode === 'country' || mode === 'lang') arr.sort((a, b) => String(a.name || '').localeCompare(String(b.name || '')));
    sub.innerHTML = `<option value="">-- ${anyTxt} --</option>` + arr.map(x => {
      const tag = (mode === 'country' && x.iso_3166_1) ? `${x.iso_3166_1} - ` : '';
      return `<option value="${escHtml(x[valKey])}">${escHtml(tag + x.name)} (${x.stationcount})</option>`;
    }).join('');
  } catch (e) { sub.innerHTML = `<option value="">Failed (${escHtml(e.message)})</option>`; }
}
async function searchRb() {
  const sel = document.getElementById('rbSel');
  sel.innerHTML = '<option value="">Searching...</option>';
  const mode = document.getElementById('rbMode').value, sub = document.getElementById('rbSub').value || '', q = document.getElementById('rbQ').value.trim();
  let path = '/json/stations/search?hidebroken=true&order=votes&reverse=true&limit=80';
  if (mode === 'topvote') path = '/json/stations/topvote/80?hidebroken=true';
  else if (mode === 'topclick') path = '/json/stations/topclick/80?hidebroken=true';
  else {
    if (mode === 'country' && sub) path += '&countrycode=' + encodeURIComponent(sub);
    if (mode === 'tag'     && sub) path += '&tag='         + encodeURIComponent(sub);
    if (mode === 'lang'    && sub) path += '&language='    + encodeURIComponent(sub);
    if (mode === 'codec'   && sub) path += '&codec='       + encodeURIComponent(sub);
    if (q)                         path += '&name='        + encodeURIComponent(q);
  }
  try {
    const list = await rbFetchJson(path);
    sel._items = list;
    sel.innerHTML = list.length ? list.map((s, i) => {
      const info = [s.codec, s.bitrate ? s.bitrate + 'k' : ''].filter(Boolean).join(' ');
      return `<option value="${i}">${escHtml(info ? `${s.name} [${info}]` : s.name)}</option>`;
    }).join('') : '<option value="">No matching stations</option>';
    onRbStationSelect();
  } catch (e) { sel.innerHTML = `<option value="">Search failed (${escHtml(e.message)})</option>`; }
}
function onRbStationSelect() {
  const sel = document.getElementById('rbSel'), item = sel._items && sel._items[sel.value];
  document.getElementById('rbMetaBox').textContent = item
    ? [item.country, item.codec, item.bitrate ? item.bitrate + ' kbps' : '', item.hls ? 'HLS' : ''].filter(Boolean).join(' \u2022 ') : '';
}
async function previewRbSelected() {
  const sel = document.getElementById('rbSel'), item = sel._items && sel._items[sel.value];
  if (!item) return;
  const url = item.url_resolved || item.url;
  if (!url) return;
  if (item.stationuuid) fetch(rbUrl(rbActiveDomain, `/json/url/${encodeURIComponent(item.stationuuid)}`)).catch(() => {});
  const name = item.name.substring(0, 31);
  const r = await fetch('/rb_preview', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:`name=${encodeURIComponent(name)}&url=${encodeURIComponent(url)}`});
  if (r.ok) { document.getElementById('rbAudTitle').textContent = 'Previewing: ' + name; updateAppendAvailability(); document.getElementById('rbAuditionBar').style.display = 'block'; }
}
async function commitRbPreview() {
  if (favsFull) return;
  const r = await fetch('/rb_commit', {method:'POST'});
  if (r.ok) { document.getElementById('rbAuditionBar').style.display = 'none'; loadFavs(); }
  else if (r.status === 409) { favsFull = true; updateAppendAvailability(); }
}
async function stopRbPreview() { await fetch('/rb_stop', {method:'POST'}); document.getElementById('rbAuditionBar').style.display = 'none'; }
async function syncAuditionState() {
  try {
    const st = await (await fetch('/rb_status')).json();
    if (st.active) { document.getElementById('rbAudTitle').textContent = 'Previewing: ' + st.name; updateAppendAvailability(); document.getElementById('rbAuditionBar').style.display = 'block'; }
  } catch (e) {}
}
</script>
)rawliteral";

#endif
// END OF FILE
