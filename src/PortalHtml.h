#ifndef PORTAL_HTML_H
#define PORTAL_HTML_H

#include <Arduino.h>
#include "PortalAssets.h"

const char HTML_HEAD[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Streaming Radio Manager</title>
  <style>
)rawliteral";

const char HTML_MID_1[] PROGMEM = R"rawliteral(
  </style>
</head>
<body>
<div class="wrap">
  <div class="topbar">
    <h1>Streaming Radio &mdash; HLS Control Portal</h1>
    <div class="tabs">
      <button class="tab active" data-tab="tabFavs" onclick="showTab('tabFavs',this)">Stations &amp; Favourites</button>
      <button class="tab" data-tab="tabWifi" onclick="showTab('tabWifi',this)">Wi-Fi Manager</button>
      <button class="tab" data-tab="tabBackup" onclick="showTab('tabBackup',this)">Backup &amp; Restore</button>
    </div>
  </div>

  <div id="tabFavs" class="pane active">
    <div class="stack">
      <div class="card">
        <h2>Add BBC Station to Favourites</h2>
        <label>Step 1: Choose Target Favourite Slot (1&ndash;10)</label>
        <select id="favSlotSel">
          <option value="0">Slot 1</option><option value="1">Slot 2</option>
          <option value="2">Slot 3</option><option value="3">Slot 4</option>
          <option value="4">Slot 5</option><option value="5">Slot 6</option>
          <option value="6">Slot 7</option><option value="7">Slot 8</option>
          <option value="8">Slot 9</option><option value="9">Slot 10</option>
        </select>
        <label>Step 2: Filter Live BBC Gist Playlist</label>
        <input type="text" id="q" placeholder="Type e.g. Radio 4, 6 Music, Gloucestershire..." oninput="filterBbc()">
        <label>Step 3: Select Station &amp; Save</label>
        <div class="row">
          <select id="bbcSel"><option value="">Loading live BBC Gist...</option></select>
          <button class="btn btn-green btn-inline" onclick="saveSelected()">Save</button>
        </div>
        <p class="hint">Fetches live stream URLs directly from the maintained BBC HLS Gist.</p>
      </div>

      <div class="card">
        <h2>Community Radio Browser (Global)</h2>
        <label>Step 1: Search by Station Name or Browse Category</label>
        <div class="row">
          <input type="text" id="rbQ" placeholder="Search station name..." onkeydown="if(event.key==='Enter')searchRb()">
          <button class="btn btn-slate btn-inline" onclick="searchRb()">Search</button>
        </div>
        <select id="rbMode" onchange="onRbModeChange()">
          <option value="country">By Country</option>
          <option value="tag">By Genre / Tag</option>
          <option value="lang">By Language</option>
          <option value="codec">By Codec (MP3 / AAC / FLAC)</option>
          <option value="topvote">Top Voted Stations</option>
          <option value="topclick">Most Clicked Stations</option>
        </select>
        <select id="rbSub" onchange="searchRb()">
          <option value="">Loading categories...</option>
        </select>
        <label>Step 2: Select Discovered Station</label>
        <select id="rbSel" onchange="onRbStationSelect()">
          <option value="">Choose a category or search above...</option>
        </select>
        <div id="rbMetaBox" class="hint" style="margin-top:8px;"></div>
        <div class="row" style="margin-top:10px;">
          <button id="btnRbPlay" class="btn btn-inline" style="flex:1;" onclick="previewRbSelected()">&#9654; Play Preview</button>
        </div>
        <div id="rbAuditionBar" class="aud-bar" style="display:none;">
          <div id="rbAudTitle" class="aud-title">Auditioning...</div>
          <div id="rbFullBanner" class="warn-banner" style="display:none;">
            All 10 favourite slots are full. Delete a preset on the right to enable appending.
          </div>
          <div class="row">
            <button id="btnRbAppend" class="btn btn-green btn-inline" style="flex:1;" onclick="commitRbPreview()">&#43; Append to Favourites &amp; Resume</button>
            <button class="btn btn-orange btn-inline" style="flex:1;" onclick="stopRbPreview()">&#9632; Stop &amp; Resume</button>
          </div>
        </div>
      </div>
    </div>

    <div class="card">
      <h2>Saved Favourite Presets (Slots 1&ndash;10)</h2>
      <table id="favTbl"><tr><td style="color:#888;">Loading saved presets...</td></tr></table>
    </div>
  </div>

  <div id="tabWifi" class="pane">
    <div class="card">
      <h2>Configure Wi-Fi Network</h2>
      <form action="/save" method="POST">
        <label>Step 1: Target Wi-Fi NVRAM Slot (1&ndash;10)</label>
        <select name="slot">
)rawliteral";

const char HTML_MID_2[] PROGMEM = R"rawliteral(
        </select>
        <label>Step 2: Pick Scanned Network or Type SSID Below</label>
        <select id="netSelect" onchange="document.getElementById('ssid').value=this.value">
          <option value="">-- Choose detected Wi-Fi --</option>
)rawliteral";

const char HTML_MID_3[] PROGMEM = R"rawliteral(
        </select>
        <button type="button" class="btn btn-slate" onclick="location.href='/rescan'">Rescan Nearby Wi-Fi Networks</button>
        <label>Network Name (SSID)</label>
        <input type="text" id="ssid" name="ssid" placeholder="Enter Wi-Fi SSID" required>
        <label>Password (leave blank for open hotspot)</label>
        <input type="password" name="pass" placeholder="Enter Wi-Fi Password">
        <button type="submit" class="btn">Save Wi-Fi Credentials</button>
      </form>
      <form action="/reboot" method="POST">
        <button type="submit" class="btn btn-green">Apply &amp; Reboot Radio</button>
      </form>
    </div>
    <div class="card">
      <h2>Saved Wi-Fi Credentials (Slots 1&ndash;10)</h2>
      <table>
)rawliteral";

const char HTML_FOOT[] PROGMEM = R"rawliteral(
      </table>
    </div>
  </div>

  <div id="tabBackup" class="pane">
    <div class="card">
      <h2>Export Full Configuration</h2>
      <p class="hint">Downloads a single .json file containing all 10 Wi-Fi networks, 10 Favourite stations, Volume, and Timezone.</p>
      <a href="/export" class="btn btn-teal">Download Full Backup (.json)</a>
    </div>
    <div class="card">
      <h2>Restore From Backup File</h2>
      <p class="hint">Select a previously exported .json backup file to overwrite NVRAM settings and reboot.</p>
      <form action="/import" method="POST" enctype="multipart/form-data">
        <label>Choose Backup File (.json)</label>
        <input type="file" name="backup" accept=".json,application/json" required>
        <button type="submit" class="btn btn-orange">Upload &amp; Restore Backup</button>
      </form>
    </div>
  </div>
</div>
)rawliteral";

#endif
// END OF FILE
