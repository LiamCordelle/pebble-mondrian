#!/usr/bin/env python3
"""Run with pebble-tool's Python. Verify real AppMessage/config/cache paths.

Weather uses a freshly fetched public London response, injected as a phone reply.
This does not claim that QEMU provides physical phone geolocation or health.
"""
import json
import os
from pathlib import Path
import subprocess
import threading
import time
import signal
from uuid import UUID
from libpebble2.communication import PebbleConnection
from libpebble2.communication.transports.websocket import MessageTargetPhone
from libpebble2.communication.transports.websocket.protocol import AppConfigSetup,AppConfigResponse,WebSocketPhonesimAppConfig,WebSocketPhonesimConfigResponse
from libpebble2.services.appmessage import AppMessageService,Uint32,ByteArray
from libpebble2.protocol.apps import AppRunState,AppRunStateRequest
from pebble_tool.sdk.emulator import ManagedEmulatorTransport

ROOT=Path(__file__).resolve().parents[1]
os.chdir(ROOT)
# Recover only this script's own log streamer if an earlier harness run stopped.
prior=subprocess.run(['/usr/sbin/lsof','-t',str(ROOT/'shots/emulator-data.log')],capture_output=True,text=True)
for text in prior.stdout.split():
    pid=int(text)
    if pid!=os.getpid():
        try:os.kill(pid,signal.SIGTERM)
        except ProcessLookupError:pass
keys=json.loads((ROOT/'build/js/message_keys.json').read_text())
uuid=UUID(json.loads((ROOT/'package.json').read_text())['pebble']['uuid'])
weather=(ROOT/'build/tests/live-weather.bin').read_bytes()
connection=PebbleConnection(ManagedEmulatorTransport('emery'))
connection.connect();connection.run_async()
service=AppMessageService(connection)
service._current_txid=180
sent=threading.Event()
def incoming(tid,app,data):
    if app==uuid and keys['REQUEST_WEATHER'] in data and not sent.is_set():
        request=data[keys['REQUEST_WEATHER']]
        service.send_message(uuid,{keys['WEATHER_ID']:Uint32(request),keys['WEATHER']:ByteArray(weather)})
        sent.set()
service.register_handler('appmessage',incoming)
def settings(large,weather_enabled,time_mode=0,unit=0,phone=False):
    value={'TIME_MODE':time_mode,'TEMP_UNIT':unit,'STEP_GOAL':10000,'WORLD':0,
        'WEATHER_ENABLED':bool(weather_enabled),'DEMO_FALLBACK':False,'LARGE_LABELS':bool(large)}
    if phone:
        connection.transport.send_packet(WebSocketPhonesimAppConfig(config=AppConfigSetup()),target=MessageTargetPhone())
        connection.read_transport_message(MessageTargetPhone,WebSocketPhonesimConfigResponse,timeout=10)
        connection.transport.send_packet(WebSocketPhonesimAppConfig(config=AppConfigResponse(data=json.dumps(value))),
            target=MessageTargetPhone())
    else:
        service.send_message(uuid,{keys[k]:Uint32(int(v)) for k,v in value.items()})
    time.sleep(1)
def cli(*args):
    subprocess.run(['pebble',*args,'--emulator','emery'],check=True,timeout=60)
def screenshot(name):
    cli('screenshot','--no-open',str(ROOT/'shots'/name))
env=os.environ.copy();env['PYTHONUNBUFFERED']='1'
with (ROOT/'shots/emulator-data.log').open('w') as output:
    logs=subprocess.Popen(['pebble','logs','--emulator','emery'],stdout=output,stderr=subprocess.STDOUT,env=env)
    try:
        time.sleep(1)
        connection.send_packet(AppRunState(data=AppRunStateRequest()))
        time.sleep(2)
        settings(True,False,1,1,phone=True)
        screenshot('emulator-settings.png')
        settings(True,True,1,1)
        if not sent.wait(8):raise RuntimeError('Watch did not request weather after enabling it')
        time.sleep(1)
        screenshot('emulator-weather.png')
        settings(False,True)
        screenshot('emulator-original-live.png')
        cli('emu-tap');screenshot('emulator-weather-detail.png');cli('emu-tap')
        # Restore the phone defaults while the watch already has a fresh cache.
        # Keeping phone weather off during injected-reply tests avoids QEMU's
        # synchronous GeoIP lookup blocking unrelated config events.
        settings(False,True,phone=True)
        # Same UUID: verify the cached forecast survives app restart/reinstall.
        cli('emu-button','click','back')
        cli('install')
        time.sleep(2)
        screenshot('emulator-restarted.png')
        cli('emu-bt-connection','--connected','no')
        time.sleep(27) # Pebble ConnectionService hides disconnects for 25 seconds.
        screenshot('emulator-disconnected.png')
        cli('emu-bt-connection','--connected','yes')
        time.sleep(1)
        screenshot('emulator-final.png')
        print('Live settings, correlated weather injection and restart/cache screenshots captured.',flush=True)
    finally:
        try:settings(False,True)
        except Exception:pass
        logs.terminate()
        try:logs.wait(timeout=5)
        except subprocess.TimeoutExpired:logs.kill();logs.wait()
        service.shutdown();connection.transport.ws.close()
