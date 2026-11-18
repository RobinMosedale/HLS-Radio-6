#ifndef PORTAL_ASSETS_H
#define PORTAL_ASSETS_H

#include <Arduino.h>

const char PORTAL_CSS[] PROGMEM = R"rawliteral(
  * { box-sizing: border-box; }
  body { font-family: Arial, sans-serif; background: #121212; color: #eee; margin: 0; padding: 16px; }
  .wrap { max-width: 1120px; margin: auto; }
  .topbar { display: flex; flex-wrap: wrap; align-items: center; justify-content: space-between; gap: 12px; margin-bottom: 16px; border-bottom: 1px solid #333; padding-bottom: 12px; }
  .topbar h1 { color: #4fc3f7; font-size: 20px; margin: 0; }
  .tabs { display: flex; flex-wrap: wrap; gap: 8px; }
  .tab { padding: 10px 16px; background: #263238; color: #ccc; border: 1px solid #37474f; border-radius: 6px; cursor: pointer; font-size: 14px; font-weight: bold; }
  .tab.active { background: #0288d1; color: #fff; border-color: #4fc3f7; }
  .pane { display: none; }
  .pane.active { display: grid; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); gap: 16px; }
  .stack { display: flex; flex-direction: column; gap: 16px; }
  .card { background: #1e1e1e; padding: 18px; border-radius: 8px; border: 1px solid #2c2c2c; }
  .card h2 { color: #4fc3f7; font-size: 17px; margin: 0 0 12px 0; }
  label { display: block; margin-top: 12px; font-size: 13px; color: #bbb; font-weight: bold; }
  .hint { font-size: 12px; color: #888; margin: 4px 0 0 0; }
  select, input[type=text], input[type=password], input[type=file] { width: 100%; padding: 10px; margin-top: 5px; border-radius: 4px; border: 1px solid #444; background: #2c2c2c; color: #fff; font-size: 14px; }
  .row { display: flex; gap: 8px; margin-top: 6px; }
  .row-wrap { display: flex; flex-wrap: wrap; gap: 8px; }
  .col-3 { flex: 1 1 130px; min-width: 0; }
  .btn { width: 100%; padding: 11px; margin-top: 14px; background: #0288d1; color: #fff; border: none; border-radius: 4px; font-size: 14px; font-weight: bold; cursor: pointer; text-align: center; text-decoration: none; display: inline-block; }
  .btn-green { background: #2e7d32; }
  .btn-teal { background: #00897b; }
  .btn-orange { background: #ef6c00; }
  .btn-slate { background: #37474f; margin-top: 8px; padding: 8px; font-size: 13px; }
  .btn-inline { width: auto; margin-top: 5px; padding: 10px 18px; white-space: nowrap; }
  table { width: 100%; border-collapse: collapse; font-size: 13px; }
  td { padding: 8px 6px; border-bottom: 1px solid #333; vertical-align: middle; }
  .fav-acts { display: flex; gap: 4px; justify-content: flex-end; align-items: center; }
  .btn-mv { background: #263238; color: #eee; border: 1px solid #455a64; padding: 4px 8px; border-radius: 4px; cursor: pointer; font-weight: bold; }
  .btn-mv:disabled { opacity: 0.25; cursor: not-allowed; }
  .del { color: #ff5252; text-decoration: none; font-weight: bold; padding: 4px 8px; border: 1px solid #5c2b2b; border-radius: 4px; background: transparent; cursor: pointer; }
)rawliteral";

const char PORTAL_JS[] PROGMEM = R"rawliteral(
<script>
let bbcList = [];
function escHtml(s) {
  return String(s || '').replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
}
function showTab(id, btn) {
  document.querySelectorAll('.pane').forEach(p => p.classList.remove('active'));
  document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
  document.getElementById(id).classList.add('active');
  if (btn) btn.classList.add('active');
  localStorage.setItem('radioTab', id);
}
function renderBbc(items) {
  const sel = document.getElementById('bbcSel');
  sel.innerHTML = items.length
    ? items.map((s, i) => `<option value="${i}">${escHtml(s.name)}</option>`).join('')
    : '<option value="">No matching stations</option>';
  sel._items = items;
}
function filterBbc() {
  const q = document.getElementById('q').value.toLowerCase();
  renderBbc(bbcList.filter(s => s.name.toLowerCase().includes(q)));
}
async function loadFavs() {
  try {
    const r = await fetch('/fav_list');
    const favs = await r.json();
    const activeCount = favs.filter(f => f && f.name && f.name.length > 0).length;
    let firstEmpty = -1;
    document.getElementById('favTbl').innerHTML = favs.map((f, i) => {
      const ok = f && f.name && f.name.length > 0;
      if (!ok && firstEmpty === -1) firstEmpty = i;
      const upDis = (i === 0) ? 'disabled' : '';
      const dnDis = (i >= activeCount - 1) ? 'disabled' : '';
      const acts = ok ? `<div class="fav-acts">
        <button class="btn-mv" ${upDis} onclick="moveFav(${i},-1)" title="Move Up">&uarr;</button>
        <button class="btn-mv" ${dnDis} onclick="moveFav(${i},1)" title="Move Down">&darr;</button>
        <button class="del" onclick="clearFav(${i})">Del</button>
      </div>` : '';
      return `<tr><td style="width:40px;"><strong>#${i + 1}</strong></td>
        <td>${ok ? escHtml(f.name) : '<em style="color:#777;">Empty Slot</em>'}</td>
        <td style="text-align:right;">${acts}</td></tr>`;
    }).join('');
    const slotSel = document.getElementById('favSlotSel');
    if (slotSel && firstEmpty !== -1) slotSel.value = String(firstEmpty);
  } catch (e) {}
}
async function loadBbc() {
  try {
    const r = await fetch('/bbc_search?q=');
    bbcList = await r.json();
    renderBbc(bbcList);
  } catch (e) {
    document.getElementById('bbcSel').innerHTML = '<option value="">Failed to load BBC Gist</option>';
  }
}
async function saveSelected() {
  const sel = document.getElementById('bbcSel');
  const item = sel._items && sel._items[sel.value];
  if (!item) return;
  const slot = document.getElementById('favSlotSel').value;
  await fetch('/fav_save?slot=' + slot + '&name=' + encodeURIComponent(item.name) + '&url=' + encodeURIComponent(item.url), {method:'POST'});
  loadFavs();
}
async function moveFav(slot, dir) {
  await fetch('/fav_move?slot=' + slot + '&dir=' + dir);
  loadFavs();
}
async function clearFav(slot) {
  await fetch('/fav_clear?slot=' + slot);
  loadFavs();
}
async function initRadioBrowser() {
  const fill = async (type, elId, valKey, labelKey, anyTxt) => {
    try {
      const res = await fetch('/rb_meta?type=' + type);
      const arr = await res.json();
      const sel = document.getElementById(elId);
      sel.innerHTML = `<option value="">${anyTxt}</option>` +
        arr.map(x => `<option value="${escHtml(x[valKey])}">${escHtml(x[labelKey])} (${x.stationcount})</option>`).join('');
    } catch (e) {}
  };
  await fill('countries', 'rbCountry', 'iso_3166_1', 'name', 'Any Country');
  await fill('tags',      'rbTag',     'name',       'name', 'Any Genre');
  await fill('langs',     'rbLang',    'name',       'name', 'Any Language');
}
async function searchRb() {
  const sel = document.getElementById('rbSel');
  sel.innerHTML = '<option value="">Searching radio-browser.info...</option>';
  const c = encodeURIComponent(document.getElementById('rbCountry').value);
  const t = encodeURIComponent(document.getElementById('rbTag').value);
  const l = encodeURIComponent(document.getElementById('rbLang').value);
  const q = encodeURIComponent(document.getElementById('rbQ').value.trim());
  try {
    const res = await fetch(`/rb_search?country=${c}&tag=${t}&lang=${l}&q=${q}`);
    const list = await res.json();
    sel._items = list;
    sel.innerHTML = list.length
      ? list.map((s, i) => {
          const info = [s.codec, s.bitrate ? s.bitrate + 'k' : ''].filter(Boolean).join(' ');
          const lbl = info ? `${s.name} [${info}]` : s.name;
          return `<option value="${i}">${escHtml(lbl)}</option>`;
        }).join('')
      : '<option value="">No matching stations</option>';
  } catch (e) {
    sel.innerHTML = '<option value="">Search failed</option>';
  }
}
async function saveRbSelected() {
  const sel = document.getElementById('rbSel');
  const item = sel._items && sel._items[sel.value];
  if (!item) return;
  const url = item.url_resolved || item.url;
  if (!url) return;
  const slot = document.getElementById('favSlotSel').value;
  const name = item.name.substring(0, 31);
  await fetch('/fav_save?slot=' + slot + '&name=' + encodeURIComponent(name) + '&url=' + encodeURIComponent(url), {method:'POST'});
  loadFavs();
}
window.onload = () => {
  const saved = localStorage.getItem('radioTab') || 'tabFavs';
  const btn = document.querySelector(`[data-tab="${saved}"]`);
  showTab(saved, btn);
  loadFavs();
  loadBbc();
  initRadioBrowser();
};
</script>
)rawliteral";

#endif
// END OF FILE
