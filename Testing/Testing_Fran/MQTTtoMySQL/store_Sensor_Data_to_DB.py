#------------------------------------------
#--- Author: Pradeep Singh
#--- Date: 20th January 2017
#--- Version: 1.0
#--- Python Ver: 2.7
#--- Details At: https://iotbytes.wordpress.com/store-mqtt-data-from-sensors-into-sql-database/
#------------------------------------------


import json
import time
import mysql.connector
from datetime import datetime


# SQLite DB Name


# Functions to push Sensor Data into Database

# Function to save into DB Table
def Data_Handler(jsonData):

	DB_Name =  "IoT.db"

	conn = mysql.connector.connect(host='192.168.170.85', user='root', password='IoTCloud_2019!', database='IoT')
	#Parse Data 
	
	
	json_Dict = json.loads(jsonData)
	
	DeviceID = json_Dict['I']
	LogID=json_Dict['L']
	DateTime = json_Dict['T']
	Codigo = json_Dict['C']
	Data = json_Dict['D']
	ProtocolVersion = json_Dict['V']
	
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

#===============================================================
# Master Function to Select DB Funtion based on MQTT Topic

def sensor_Data_Handler(Topic, jsonData):
	

	if Topic == "test":
		Data_Handler(jsonData)
	
				

#===============================================================