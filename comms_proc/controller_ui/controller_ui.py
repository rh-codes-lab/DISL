import os
import re
import copy
import sys
import math
from tkinter import *
from tkinter import ttk
from tkinter import filedialog as fd
import psycopg2
import paho.mqtt.client as mqtt
import json

dbhostname = '192.168.0.147'
#hostname = 'localhost'

canvas = Tk()

canvas.geometry("1000x600")
canvas.update()
width = canvas.winfo_width()
height = canvas.winfo_height()
canvas.title("Red Hat Fleet Controller")

chosen_device = ""
heartbeat_topic = ""
riscv_topic = ""
command_response_topic = ""
command_topic = ""
comms_proc_fw = "?"
softcore_fw = "?"
chosen_device_topics = ""
last_doorbell_reading = "?"
last_temperature_reading = "?"
doorbell_topic = ""
fpga_bitstream = "?"
softcore_hex = "?"

try:
    conn = psycopg2.connect("dbname='rh_things' user='rh_user' host='" + dbhostname + "' password='redhat'")
except:
    print("Unable to connect to the database")

with conn.cursor() as curs:
    curs.execute("SELECT name from types")
    device_type_list = curs.fetchall()

def on_connect(client, userdata, flags, rc):
    return

def refresh_device_info():
    device_info_text.delete("1.0", "end")
    device_info_text.insert(END, 'Comms Proc FW: ' + comms_proc_fw + '\nSoftcore FW: ' + softcore_fw +  '\nFPGA file: ' + fpga_bitstream + '\nSoftcore file: ' + softcore_hex + '\nDoorbell: ' + last_doorbell_reading + '\nTemperature: ' + last_temperature_reading)

def on_message(client, userdata, msg):
    global softcore_fw
    global softcore_hex
    global fpga_bitstream
    global comms_proc_fw
    global last_doorbell_reading
    global last_temperature_reading

    payload = str(msg.payload.decode("utf-8"))
    try:
        json_payload = json.loads(payload)
    except:
        return
    if('version' in json_payload):
        softcore_fw = json_payload['version']
        refresh_device_info()

    if (msg.topic == riscv_topic):
        last_temperature_reading = json_payload['temperature']
        refresh_device_info()
    #    riscv_text.insert(END, payload + '\n')
    #elif (msg.topic == heartbeat_topic):
    #    heartbeat_text.insert(END, payload + '\n')
    elif (msg.topic == command_response_topic):
        if (json_payload['command'] == 'ListCommands'):
            device_commands_text.delete("1.0","end")
            for item in json_payload['response']:
                device_commands_text.insert(END, item + '\n')

        elif (json_payload['command'] == 'ListSDCardFiles'):
            file_list_text.delete("1.0","end")
            for item in json_payload['response']:
                file_list_text.insert(END, item + '\n')

        elif (json_payload['command'] == 'GetVersion'):
            comms_proc_fw = json_payload['response']
            refresh_device_info()
        
        elif (json_payload['command'] == 'JTAGProgramFPGA'):
            reconfigure_fpga_text.delete(0, END)
            fpga_bitstream = json_payload['response'].split('/')[-1] 
            refresh_device_info()

        elif (json_payload['command'] == 'FlashSoftcore'):
            reprogram_softcore_text.delete(0, END)
            softcore_hex = json_payload['response'].split('/')[-1] 
            refresh_device_info()

    elif (msg.topic == doorbell_topic):
        last_doorbell_reading = 'Detected' if json_payload['detection'] == 1 else 'No Detection'
        refresh_device_info()

#            print('GetVersion gives us: ' + comms_proc_fw)

#        else:
#            command_response_text.insert(END, payload + '\n')

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

#client.connect("192.168.0.194", 1883, 60)

connected = 0

try:
    client.connect("localhost", 1883, 60)
    connected = 1
except:
    connected = 0


def send_command_command():
    command = send_command_text.get()
    client.publish(command_topic, command)

#def select_device_type_command(selection):
def get_devices():
    with conn.cursor() as curs:
        #sql_string = "SELECT name from things where type='" + selection[0] + "'"
        sql_string = "SELECT name from things"
        curs.execute(sql_string)
        device_rows = curs.fetchall()
        device_list = ['None Selected']
        for dev in device_rows:
            device_list.append(dev)
        device_select_combobox['values']=device_list
        device_select_combobox.current(0)

def select_device_command(event):
    global heartbeat_topic
    global riscv_topic
    global command_response_topic
    global command_topic
    global chosen_device
    global chosen_device_topics
    global doorbell_topic

    chosen_device = device_select_label.get()

    '''    
    if heartbeat_topic != "":
        client.unsubscribe(heartbeat_topic)
    if command_response_topic != "":
        client.unsubscribe(command_response_topic)
    if riscv_topic != "":
        client.unsubscribe(riscv_topic)
    '''
    if chosen_device_topics != "":
        client.unsubscribe(chosen_device_topics)

    #heartbeat_topic = "/" + chosen_device + "/heartbeat"
    command_response_topic = "/" + chosen_device + "/out-command"
    command_topic = "/" + chosen_device + "/in-command"
    doorbell_topic = "/" + chosen_device + "/doorbell"
    riscv_topic = "/" + chosen_device + "/risc_v"
    chosen_device_topics = "/" + chosen_device + "/#"
    #client.subscribe(heartbeat_topic)
    #client.subscribe(riscv_topic)
    #client.subscribe(command_response_topic)
    client.subscribe(chosen_device_topics)
    msg = '{"command":"ListCommands"}'
    client.publish(command_topic, msg)
    msg = '{"command":"ListSDCardFiles"}'
    client.publish(command_topic, msg)
    msg = '{"command":"GetVersion"}'
    client.publish(command_topic, msg)

def download_file_command():
    url = download_file_text.get()
    filename = url.split('/')[-1]
    command = '{"command":"GetFileFromURL","url":"' + url + '","filename":"/sdcard/' + filename + '"}'
    client.publish(command_topic, command)

#def clear_riscv():
#    riscv_text.delete("1.0", "end")

#def clear_heartbeat():
#    heartbeat_text.delete("1.0","end")

#def clear_command_response():
#    command_response_text.delete("1.0","end")

def refresh_file_list():
    command = '{"command":"ListSDCardFiles"}'
    client.publish(command_topic, command)

def refresh_device_commands_list():
    command = '{"command":"ListCommands"}'
    client.publish(command_topic,command)

def reprogram_softcore_command():
    global softcore_hex
    filename = reprogram_softcore_text.get()
    command = '{"command":"FlashSoftcore","filename":"/sdcard/' + filename + '"}'
    softcore_hex = 'updating to ' + filename
    refresh_device_info()
    client.publish(command_topic,command)

def reconfigure_fpga_command():
    global fpga_bitstream
    filename = reconfigure_fpga_text.get()
    command = '{"command":"JTAGProgramFPGA","filename":"/sdcard/' + filename + '"}'
    fpga_bitstream = 'updating to ' + filename
    refresh_device_info()
    client.publish(command_topic,command)

def connect_broker():
    connected = 1
    try:
        client.disconnect()
    except:
        connected = 0
    try:
        client.connect(mqtt_host.get(), int(mqtt_port.get()), 60)
    except:
        print("Failed to connect to host: " + mqtt_host.get() + " on port: " + mqtt_port.get())
        connected = 0


rownumber = 0

mqtt_host_text_frame = LabelFrame(canvas, text="MQTT Broker")
mqtt_host_text_frame.grid(column=0, row=rownumber, padx=10, pady=10)

mqtt_host = StringVar()
mqtt_host_text = Entry(mqtt_host_text_frame, textvariable=mqtt_host)
mqtt_host_text.insert(0, "localhost")
mqtt_host_text.grid(column=0, row=0)


mqtt_port = StringVar()
mqtt_port_text = Entry(mqtt_host_text_frame, textvariable=mqtt_port)
mqtt_port_text.insert(0, "1883")
mqtt_port_text.grid(column=1, row=0)

mqtt_host_connect_button = Button(mqtt_host_text_frame, text = "Connect", command = connect_broker)
mqtt_host_connect_button.grid(column=2, row=0)

device_select_frame = LabelFrame(canvas, text="Device")
device_select_frame.grid(column=1, row=rownumber, padx=10, pady=10)
device_select_label = StringVar()
device_select_label.set('')
device_select_combobox = ttk.Combobox(device_select_frame, textvariable=device_select_label, width=35)
device_select_combobox['state'] = 'readonly'
device_select_combobox.pack()
device_select_combobox.bind("<<ComboboxSelected>>", select_device_command)

get_devices()

rownumber = rownumber+1

device_info_frame = LabelFrame(canvas, text="Device Status")
device_info_frame.grid(column = 0, row=rownumber, padx=10, pady=10)
device_info_text = Text(device_info_frame, height=6, width = 45)
device_info_text.grid(column=0, row=0)

rownumber = rownumber+1
columnnumber = 0

device_commands_frame = LabelFrame(canvas, text="Available device commands")
device_commands_frame.grid(column=0, row=rownumber, padx=10, pady=10)
device_commands_text = Text(device_commands_frame, height=10, width = 35)
device_commands_text.grid(column=0, row=0)
device_commands_refresh_button = Button(device_commands_frame, text = "Refresh", command = refresh_device_commands_list)
device_commands_refresh_button.grid(column=1, row=0)
columnnumber = columnnumber+1

file_list_frame = LabelFrame(canvas, text="Device files")
file_list_frame.grid(column=columnnumber, row=rownumber, padx=10, pady=10)
file_list_text = Text(file_list_frame, height=10, width=35)
file_list_text.grid(column=0, row=0)
file_list_refresh_button = Button(file_list_frame, text = "Refresh", command = refresh_file_list)
file_list_refresh_button.grid(column=1, row=0)

rownumber = rownumber+1

send_command_frame = LabelFrame(canvas, text="Send command to device")
send_command_frame.grid(column=0, row=rownumber, padx=10, pady=10)
send_command_text = Entry(send_command_frame, width=35)
send_command_text.grid(column=0, row=0)
send_command_button = Button(send_command_frame, text = "Send", command = send_command_command)
send_command_button.grid(column=1, row=0)

download_file_frame = LabelFrame(canvas, text="Device download file from URL")
download_file_frame.grid(column=1, row=rownumber, padx=10, pady=10)
download_file_text = Entry(download_file_frame, width=35)
download_file_text.grid(column=0, row=0)
download_file_button = Button(download_file_frame, text = "Transfer", command = download_file_command)
download_file_button.grid(column=1, row=0)

rownumber = rownumber+1

reconfigure_fpga_frame = LabelFrame(canvas, text="Reconfigure FPGA with filename")
reconfigure_fpga_frame.grid(column=0, row=rownumber, padx=10, pady=10)
reconfigure_fpga_text = Entry(reconfigure_fpga_frame, width=35)
reconfigure_fpga_text.grid(column=0, row=0)
reconfigure_fpga_button = Button(reconfigure_fpga_frame, text = "Reconfigure", command = reconfigure_fpga_command)
reconfigure_fpga_button.grid(column=1, row=0)

reprogram_softcore_frame = LabelFrame(canvas, text="Reprogram softcore with filename")
reprogram_softcore_frame.grid(column=1, row=rownumber, padx=10, pady=10)
reprogram_softcore_text = Entry(reprogram_softcore_frame, width=35)
reprogram_softcore_text.grid(column=0, row=0)
reprogram_softcore_button = Button(reprogram_softcore_frame, text = "Reprogram", command = reprogram_softcore_command)
reprogram_softcore_button.grid(column=1, row=0)


'''
rownumber = rownumber+1

riscv_frame = LabelFrame(canvas, text="RISC-V messages")
riscv_frame.grid(column=1, row=rownumber, padx=20, pady=10)
riscv_text = Text(riscv_frame, width=30, height=10)
riscv_text.grid(column=0, row=0)
riscv_clear_button = Button(riscv_frame, text = "Clear", command = clear_riscv)
riscv_clear_button.grid(column=1, row=0)

#heartbeat_frame = LabelFrame(canvas, text="Heartbeat from device")
heartbeat_frame.grid(column=2, row=rownumber, padx=20, pady=10)
heartbeat_text = Text(heartbeat_frame, width=30, height=10)
heartbeat_text.grid(column=0, row=0)
heartbeat_clear_button = Button(heartbeat_frame, text = "Clear", command = clear_heartbeat)
heartbeat_clear_button.grid(column=1, row=0)

command_response_frame = LabelFrame(canvas, text="Command responses from device")
command_response_frame.grid(column=0, row=rownumber, padx=20, pady=10)
command_response_text = Text(command_response_frame, width=30, height=10)
command_response_text.grid(column=0, row=0)
command_response_clear_button = Button(command_response_frame, text = "Clear", command = clear_command_response)
command_response_clear_button.grid(column=1, row=0)
'''

client.loop_start()
canvas.mainloop()

