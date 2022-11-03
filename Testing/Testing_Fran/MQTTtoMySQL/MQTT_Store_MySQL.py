
import paho.mqtt.client as mqtt
import json
import time
import mysql.connector
from datetime import datetime

# MQTT Settings 
MQTT_Broker = "192.168.170.85"
MQTT_Port = 1883
Keep_Alive_Interval = 45
MQTT_Topic = "test"

#Subscribe to all Sensors at Base Topic
def on_connect(self, mosq, obj, rc):
	self.subscribe(MQTT_Topic, 0)

#Save Data into DB Table
def on_message(mosq, obj, msg):
	# This is the Master Call for saving MQTT Data into DB
	# For details of "sensor_Data_Handler" function please refer "sensor_data_to_db.py"
	#print ("MQTT Data Received...")
	#print ("MQTT Topic: " + msg.topic)  
	#print ("Data: " + msg.payload)
	Data_Handler( msg.payload)

def on_subscribe(mosq, obj, mid, granted_qos):

    pass

def Data_Handler(jsonData):

	DB_Name =  "IoT.db"

	conn = mysql.connector.connect(host='192.168.170.85', user='root', password='IoTCloud_2019!', database='IoT')
	#Parse Data 
	
	
	json_Dict = json.loads(jsonData)
	
	DeviceID = json_Dict.get('I')
	LogID=json_Dict.get('L')
	DateTime = json_Dict.get('T')
	Codigo = json_Dict.get('C')
	Data = json_Dict.get('D')
	ProtocolVersion = json_Dict.get('V')
	
	DateTime = time.strftime('%Y-%m-%d %H:%M:%S', time.gmtime(DateTime))
	cursor = conn.cursor()
	#Push into DB Table
	consultaSQL="insert into Data ( `DeviceID`, `LogID`, `FechaHoraData`, `Codigo`, `Data`, `Version`) values (" + str(DeviceID) + "," + str(LogID) + ",'" + str(DateTime) + "'," + str(Codigo) + ",'"+  str(Data) + "'," + str(ProtocolVersion) + ")"
	cursor.execute(consultaSQL)
	#print consultaSQL
	#print ("Inserted Sensor Data into Database.")
	#print ("")
	conn.commit()
	cursor.close()
	conn.close()

mqttc = mqtt.Client()

# Assign event callbacks
mqttc.on_message = on_message
mqttc.on_connect = on_connect
mqttc.on_subscribe = on_subscribe

# Connect
mqttc.connect(MQTT_Broker, int(MQTT_Port), int(Keep_Alive_Interval))

#Continue the network loop
mqttc.loop_forever()