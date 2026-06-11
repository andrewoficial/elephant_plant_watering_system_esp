#ifndef HTML_PAGE_H
#define HTML_PAGE_H

const char* const index_html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Метеостанция</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body { font-family: 'Segoe UI', Roboto, sans-serif; background: #2a5a3a; color: #e0f0e0; padding: 16px; }
        .container { max-width: 1300px; margin: 0 auto; }
        .header { background: rgba(30, 59, 44, 0.8); padding: 12px 20px; margin-bottom: 24px; display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; border: 1px solid #7cb342; gap: 10px; }
        .time-box, .temp-box { background: rgba(30, 59, 44, 0.85); padding: 8px 16px; border: 1px solid #9ccc65; }
        .time-box { font-size: 1.4rem; } .temp-box { font-size: 1.5rem; }
        .controls-box { display: flex; align-items: center; gap: 15px; font-size: 0.9rem; }
        .clear-btn { background: #b71c1c; color: white; border: 1px solid #f44336; padding: 8px 16px; cursor: pointer; }
        .device-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(380px, 1fr)); gap: 20px; margin-bottom: 30px; }
        .device-card { background: rgba(42, 90, 58, 0.9); padding: 16px; border-left: 4px solid #9ccc65; }
        .device-header { display: flex; justify-content: space-between; align-items: baseline; flex-wrap: wrap; border-bottom: 1px solid #7cb342; padding-bottom: 8px; margin-bottom: 12px; }
        .device-addr { font-weight: bold; background: #1e3b2c; padding: 2px 8px; font-family: monospace; }
        .device-name { font-weight: bold; background: #1e3b2c; padding: 2px 8px; }
        .last-seen { font-size: 0.8rem; margin-top: 6px; color: #c8e6b5; text-align: right; }
        .data-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin: 12px 0; }
        .data-item { background: #204e34; padding: 6px 10px; } .data-item-full { background: #204e34; padding: 6px 10px; grid-column: span 2; }
        .data-label { font-size: 0.85rem; opacity: 0.9; } .data-value { font-weight: bold; font-size: 1.1rem; }
        .pump-on { color: #ffeb3b; } .pump-off { color: #81c784; } .success { color: #81c784; } .fail { color: #f44336; }
        .section { border-top: 1px dashed #7cb342; margin-top: 12px; padding-top: 8px; }
        .section-title { font-weight: bold; margin-bottom: 8px; font-size: 1rem; }
        .schedule-item { background: #1e3b2c; padding: 8px; margin-bottom: 6px; display: flex; justify-content: space-between; align-items: center; border-left: 3px solid #9ccc65; }
        .sch-text { font-size: 0.9rem; flex-grow: 1; line-height: 1.4; }
        .sch-del { background: #b71c1c; color: white; border: none; padding: 2px 6px; cursor: pointer; margin-left: 8px; }
        .sch-form { display: flex; flex-wrap: wrap; gap: 5px; align-items: center; font-size: 0.85rem; margin-top: 6px; }
        .sch-form input, .sch-form select, .sch-form button { padding: 4px 6px; background: #2e5e3e; border: 1px solid #9ccc65; color: white; }
        .sch-form button { cursor: pointer; background: #3b7a54; }
        .cmd-form { margin-top: 12px; background: #204e34; padding: 10px; font-size: 0.85rem; display: flex; flex-wrap: wrap; gap: 6px; align-items: center; }
        .cmd-form select, .cmd-form input, .cmd-form button { padding: 4px 8px; background: #2e5e3e; border: 1px solid #9ccc65; color: white; }
        .cmd-form button { cursor: pointer; background: #3b7a54; } .cmd-form select { min-width: 140px; }
        .footer { text-align: center; margin-top: 30px; font-size: 0.75rem; opacity: 0.7; }
    </style>
</head>
<body>
<div class="container">
    <div class="header">
        <div class="time-box" id="localTime">--:--:--</div>
        <div class="temp-box" id="localTemp">🌡️ --°C</div>
        <div class="controls-box">
            <label><input type="checkbox" id="autoRefresh" checked> Авто</label>
            <button class="clear-btn" onclick="clearDevices()">🗑️ Очистить</button>
        </div>
    </div>
    <div class="device-grid" id="devicesGrid">Загрузка...</div>
    <div class="footer">🌿 Полив по расписанию | ADVRT Protocol | обновление: 5 сек</div>
</div>
<script>
let knownDevices = [];
function formatAgo(s) { if(s<60) return s+" сек"; if(s<3600) return Math.floor(s/60)+" мин"; return Math.floor(s/3600)+" ч"; }

function parseFlags(f) {
    if(!f||f.length<16) return ''; let h='<div style="display:flex;flex-wrap:wrap;gap:4px;font-size:0.75rem;margin-top:6px;">';
    if(f[15]=='1') h+='<span style="background:#f44336;color:#fff;padding:2px 5px;border-radius:3px;">EEPROM ERR</span>';
    if(f[14]=='1') h+='<span style="background:#f44336;color:#fff;padding:2px 5px;border-radius:3px;">HC12 ERR</span>';
    if(f[13]=='1') h+='<span style="background:#f44336;color:#fff;padding:2px 5px;border-radius:3px;">DHT ERR</span>';
    if(f[12]=='1') h+='<span style="background:#f44336;color:#fff;padding:2px 5px;border-radius:3px;">RELAY ERR</span>';
    if(f[11]=='1') h+='<span style="background:#f44336;color:#fff;padding:2px 5px;border-radius:3px;">SOIL SEN ERR</span>';
    if(f[10]=='1') h+='<span style="background:#9c27b0;color:#fff;padding:2px 5px;border-radius:3px;">MOTOR EMER</span>';
    if(f[9]=='1')  h+='<span style="background:#9c27b0;color:#fff;padding:2px 5px;border-radius:3px;">MOTOR MAN OFF</span>';
    if(f[6]=='1')  h+='<span style="background:#ff9800;color:#000;padding:2px 5px;border-radius:3px;">T SIM</span>';
    if(f[5]=='1')  h+='<span style="background:#ff9800;color:#000;padding:2px 5px;border-radius:3px;">H SIM</span>';
    if(f[4]=='1')  h+='<span style="background:#ff9800;color:#000;padding:2px 5px;border-radius:3px;">P SIM</span>';
    if(f[3]=='1')  h+='<span style="background:#ff9800;color:#000;padding:2px 5px;border-radius:3px;">S SIM</span>';
    if(f[2]=='1')  h+='<span style="background:#4caf50;color:#fff;padding:2px 5px;border-radius:3px;">PUMP WORK</span>';
    if(f[1]=='1')  h+='<span style="background:#4caf50;color:#fff;padding:2px 5px;border-radius:3px;">SOIL PWR</span>';
    return h+'</div>';
}

function getCondText(c) { if(c===1) return "выше"; if(c===-1) return "ниже"; return "любая"; }

function formatScheduleText(sc) {
    let totalMins = Math.round(sc.intv);
    let freq = "";
    
    if (totalMins === 0.25) freq = "Каждые 15 сек";
    else if (totalMins === 60) freq = "Каждый час";
    else if (totalMins === 720) freq = "Каждые 12 ч";
    else if (totalMins === 1440) freq = "Каждый день";
    else if (totalMins === 2880) freq = "Каждые 2 дня";
    else if (totalMins === 4320) freq = "Каждые 3 дня";
    else if (totalMins === 10080) freq = "Каждую неделю";
    else {
        let days = Math.floor(totalMins / 1440);
        let hours = Math.floor((totalMins % 1440) / 60);
        let mins = totalMins % 60;
        freq = "Каждые ";
        if (days > 0) freq += days + " дн ";
        if (hours > 0) freq += hours + " ч ";
        if (mins > 0) freq += mins + " мин";
    }

    // Защита от undefined: если hour не пришел с сервера, считаем что -1
    let hour = (sc.hour !== undefined && sc.hour !== null) ? sc.hour : -1;
    let timeStr = hour == -1 ? "" : " в " + String(hour).padStart(2, '0') + ":00";
    let condTxt = sc.cond === 0 ? "" : ", если темп " + getCondText(sc.cond) + " " + sc.thr + "°C";
    
    return freq + timeStr + condTxt;
}

function renderSchedules(addr, schedules) {
    if (!schedules) return ''; let h = '';
    schedules.forEach(sc => {
        h += `<div class="schedule-item">
            <div class="sch-text">${formatScheduleText(sc)}</div>
            <button class="sch-del" onclick="delSchedule('${addr}', ${sc.id})">✖</button>
        </div>`;
    });
    return h;
}

function renderCard(dev) {
    let pumpClass = dev.pumpOn ? 'pump-on' : 'pump-off'; let pumpText = dev.pumpOn ? '🟡 ВКЛ' : '🟢 ВЫКЛ';
    let sucClass = dev.lastWatSuccess ? 'success' : 'fail'; let sucText = dev.lastWatSuccess ? 'Успешно' : 'Неуспешно';
    
    let hourOptions = '<option value="-1">Любое время</option>';
    for(let i=0; i<24; i++) hourOptions += `<option value="${i}">${String(i).padStart(2,'0')}:00</option>`;

    return `
    <div class="device-card" id="card_${dev.addr}">
        <div class="device-header">
            <span><span class="device-addr">${dev.addr}</span> <span class="device-name" id="nameDisplay_${dev.addr}">${dev.name}</span></span>
            <span>${dev.interface}</span>
        </div>
        <div class="last-seen" id="lastSeen_${dev.addr}">📡 ${formatAgo(dev.lastSeenAgo)} назад</div>
        
        <div class="data-grid">
            <div class="data-item"><div class="data-label">🌡️ Темп. воздуха</div><div class="data-value" id="tempAir_${dev.addr}">${dev.tempAir > -900 ? dev.tempAir + '°C' : '—'}</div></div>
            <div class="data-item"><div class="data-label">💧 Влажность воздуха</div><div class="data-value" id="humAir_${dev.addr}">${dev.humAir > -900 ? dev.humAir + '%' : '—'}</div></div>
            <div class="data-item"><div class="data-label">📉 Давление</div><div class="data-value" id="pressure_${dev.addr}">${dev.pressure > -900 ? dev.pressure + ' hPa' : '—'}</div></div>
            <div class="data-item"><div class="data-label">💧 Влажность почвы</div><div class="data-value" id="humSoil_${dev.addr}">${dev.humSoil > -900 ? dev.humSoil : '—'}</div></div>
            <div class="data-item"><div class="data-label">🚿 Статус помпы</div><div class="data-value ${pumpClass}" id="pump_${dev.addr}">${pumpText}</div></div>
            <div class="data-item"><div class="data-label">📈 Ср. темп. дня</div><div class="data-value" id="avgTemp_${dev.addr}">${dev.avgDailyTemp > -900 ? dev.avgDailyTemp.toFixed(1) + '°C' : '—'}</div></div>
            <div class="data-item-full"><div class="data-label">⏱ Время работы</div><div class="data-value" id="workTime_${dev.addr}">${dev.workTime || '—'}</div></div>
        </div>
        <div id="flags_${dev.addr}">${parseFlags(dev.flags)}</div>

        <div class="section">
            <div class="section-title">💧 Информация о поливе</div>
            <div style="font-size:0.9rem; line-height:1.6;">
                Успех последнего полива: <span class="${sucClass}" id="watSuc_${dev.addr}">${sucText}</span><br>
                Последний полив (по таймеру платы): <span id="lastWatTime_${dev.addr}">${dev.lastWatTime || '—'}</span><br>
                Последний полив (по системе ESP32): <span id="lastWateringSystem_${dev.addr}">${dev.lastWateringSystem}</span> назад<br>
                <strong>До следующего полива: <span id="nextWatIn_${dev.addr}">${dev.nextWateringIn}</span></strong>
            </div>
        </div>

        <div class="section">
            <div class="section-title">📅 Расписание</div>
            <div id="scheduleList_${dev.addr}">${renderSchedules(dev.addr, dev.schedules)}</div>
            <div class="sch-form" style="flex-direction: column; align-items: stretch;">
                <div style="display:flex; gap:5px; flex-wrap:wrap; align-items: center;">
                    <select id="schFreq_${dev.addr}" onchange="onFreqChange('${dev.addr}')">
                        <option value="0.25">Каждые 15 сек (Тест)</option>
                        <option value="60">Каждый час</option>
                        <option value="720">Каждые 12 часов</option>
                        <option value="1440" selected>Каждый день</option>
                        <option value="2880">Каждые 2 дня</option>
                        <option value="4320">Каждые 3 дня</option>
                        <option value="10080">Каждую неделю</option>
                        <option value="custom">Другой интервал...</option>
                    </select>
                    
                    <span id="schCustomBlock_${dev.addr}" style="display:none; gap:5px; align-items:center;">
                        <input type="number" id="schCustD_${dev.addr}" value="0" min="0" style="width:40px" title="Дни"> д
                        <input type="number" id="schCustH_${dev.addr}" value="0" min="0" max="23" style="width:35px" title="Часы"> ч
                    </span>

                    <span style="margin-left:auto;">в</span>
                    <select id="schHour_${dev.addr}">${hourOptions}</select>
                </div>
                <div style="display:flex; gap:5px; flex-wrap:wrap; margin-top:5px; align-items:center;">
                    <select id="schCond_${dev.addr}">
                        <option value="0">Всегда</option>
                        <option value="1">Если темп ></option>
                        <option value="-1">Если темп <</option>
                    </select>
                    <input type="number" id="schThr_${dev.addr}" value="25" step="1" style="width:50px" placeholder="°C">°C
                    <button onclick="addSchedule('${dev.addr}')">➕ Добавить</button>
                </div>
            </div>
        </div>

        <div class="section">
            <div class="section-title">🛠 Управление</div>
            <div class="sch-form">
                <input type="text" id="nameInput_${dev.addr}" value="${dev.name}" size="10" placeholder="Имя цветка">
                <button onclick="setName('${dev.addr}')">💾 Задать имя</button>
            </div>
            <div class="cmd-form">
                <select id="cmdSel_${dev.addr}">
                    <option value="ADVRT">ADVRT (Опрос)</option>
                    <option value="REBOOT">REBOOT</option>
                    <option value="PUMP ON">PUMP ON</option>
                    <option value="PUMP OFF">PUMP OFF</option>
                    <option value="PUMP ">PUMP (сек)</option>
                    <option value="CERR?">CERR?</option><option value="CERR RESET">CERR RESET</option>
                    <option value="TERM?">TERM?</option><option value="TERM ">TERM (зн)</option>
                    <option value="HYDM?">HYDM?</option><option value="HYDM ">HYDM (зн)</option>
                    <option value="HYGR?">HYGR?</option><option value="HYGR DELTA ">HYGR DELTA</option>
                    <option value="SGDE ">SGDE</option><option value="GGDE">GGDE</option>
                    <option value="SGTI?">SGTI?</option><option value="SGTI ">SGTI</option>
                    <option value="WBAUD?">WBAUD?</option><option value="HBAUD?">HBAUD?</option>
                </select>
                <input type="text" id="cmdArg_${dev.addr}" placeholder="Аргумент" size="8">
                <button onclick="sendCmd('${dev.addr}')">⚡ Отправить</button>
            </div>
        </div>
    </div>`;
}

function onFreqChange(addr) {
    let freq = document.getElementById(`schFreq_${addr}`).value;
    let customBlock = document.getElementById(`schCustomBlock_${addr}`);
    let hourSel = document.getElementById(`schHour_${addr}`);
    
    customBlock.style.display = (freq === "custom") ? "flex" : "none";
    
    // Если интервал 15 сек или 1 час, выбор часа суток не имеет смысла
    if (freq === "0.25" || freq === "60") { 
        hourSel.value = "-1"; 
        hourSel.disabled = true;
    } else {
        hourSel.disabled = false;
    }
}

function updateCardData(dev) {
    let d = id => document.getElementById(id); let el;
    el=d(`tempAir_${dev.addr}`); if(el) el.innerText = dev.tempAir > -900 ? dev.tempAir + '°C' : '—';
    el=d(`humAir_${dev.addr}`); if(el) el.innerText = dev.humAir > -900 ? dev.humAir + '%' : '—';
    el=d(`pressure_${dev.addr}`); if(el) el.innerText = dev.pressure > -900 ? dev.pressure + ' hPa' : '—';
    el=d(`humSoil_${dev.addr}`); if(el) el.innerText = dev.humSoil > -900 ? dev.humSoil : '—';
    el=d(`avgTemp_${dev.addr}`); if(el) el.innerText = dev.avgDailyTemp > -900 ? dev.avgDailyTemp.toFixed(1) + '°C' : '—';
    el=d(`pump_${dev.addr}`); if(el) { el.innerText = dev.pumpOn ? '🟡 ВКЛ' : '🟢 ВЫКЛ'; el.className = 'data-value ' + (dev.pumpOn ? 'pump-on' : 'pump-off'); }
    el=d(`watSuc_${dev.addr}`); if(el) { el.innerText = dev.lastWatSuccess ? 'Успешно' : 'Неуспешно'; el.className = dev.lastWatSuccess ? 'success' : 'fail'; }
    el=d(`workTime_${dev.addr}`); if(el) el.innerText = dev.workTime || '—';
    el=d(`lastWatTime_${dev.addr}`); if(el) el.innerText = dev.lastWatTime || '—';
    el=d(`lastWateringSystem_${dev.addr}`); if(el) el.innerText = dev.lastWateringSystem + ' назад';
    el=d(`nextWatIn_${dev.addr}`); if(el) el.innerText = dev.nextWateringIn;
    el=d(`flags_${dev.addr}`); if(el) el.innerHTML = parseFlags(dev.flags);
    el=d(`lastSeen_${dev.addr}`); if(el) el.innerText = '📡 ' + formatAgo(dev.lastSeenAgo) + ' назад';
    el=d(`nameDisplay_${dev.addr}`); if(el) el.innerText = dev.name;
    el=d(`scheduleList_${dev.addr}`); if(el) el.innerHTML = renderSchedules(dev.addr, dev.schedules);
}

async function apiPost(action, payload) { const res = await fetch('/api/config', { method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify(payload) }); if(res.ok) fetchData(); else alert("Ошибка"); }
function setName(addr) { let name = document.getElementById(`nameInput_${addr}`).value; apiPost('setName', {addr, action: 'setName', name}); }

function addSchedule(addr) { 
    let freqSel = document.getElementById(`schFreq_${addr}`);
    let intv = 1440;
    
    if (freqSel.value === "custom") {
        let d = parseFloat(document.getElementById(`schCustD_${addr}`).value) || 0;
        let h = parseFloat(document.getElementById(`schCustH_${addr}`).value) || 0;
        intv = d * 1440 + h * 60;
        if (intv <= 0) { alert("Интервал должен быть больше 0"); return; }
    } else {
        intv = parseFloat(freqSel.value);
    }
    
    let hour = parseInt(document.getElementById(`schHour_${addr}`).value); 
    let cond = parseInt(document.getElementById(`schCond_${addr}`).value); 
    let thr = parseFloat(document.getElementById(`schThr_${addr}`).value); 
    apiPost('addSchedule', {addr, action: 'addSchedule', intv, hour, cond, thr}); 
}
function delSchedule(addr, scId) { if(confirm('Удалить это правило?')) apiPost('delSchedule', {addr, action: 'delSchedule', delId: scId}); }

async function sendCmd(addr) {
    let cmdBase = document.getElementById(`cmdSel_${addr}`).value; let cmdArg = document.getElementById(`cmdArg_${addr}`).value.trim(); let fullCmd = cmdBase;
    if (cmdArg.length > 0 && cmdBase.endsWith(' ')) fullCmd = cmdBase + cmdArg; else if (cmdArg.length === 0 && cmdBase.endsWith(' ')) fullCmd = cmdBase.trim();
    if(confirm(`Отправить '${fullCmd}' на ${addr}?`)) { const res = await fetch('/api/cmd', { method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify({ addr, cmd: fullCmd }) }); if (!res.ok) alert("Ошибка"); }
}
async function clearDevices() { if(confirm('Очистить базу?')) { const res = await fetch('/api/clear', { method: 'POST' }); if(res.ok) fetchData(); } }

async function fetchData() {
    try {
        const res = await fetch('/api/data'); const data = await res.json();
        document.getElementById('localTime').innerText = data.time; document.getElementById('localTemp').innerHTML = '🌡️ ' + data.localTemp + '°C';
        if (!document.getElementById('autoRefresh').checked) return;
        const grid = document.getElementById('devicesGrid');
        if (!data.devices.length) { grid.innerHTML = '<div style="text-align:center; padding:20px;">Нет активных устройств</div>'; knownDevices=[]; return; }
        let currentAddrs = data.devices.map(d => d.addr).sort(); let needsRebuild = (currentAddrs.length !== knownDevices.length) || currentAddrs.some((v, i) => v !== knownDevices[i]);
        if (needsRebuild) { knownDevices = currentAddrs; let html = ''; for (let dev of data.devices) html += renderCard(dev); grid.innerHTML = html; }
        else { for (let dev of data.devices) updateCardData(dev); }
    } catch(e) { console.error(e); }
}
fetchData(); setInterval(fetchData, 5000);
</script>
</body>
</html>
)rawliteral";
#endif