#!/usr/bin/env python
import pika, sys, os
import json
import time
import mysql.connector

def main():
    connection = pika.BlockingConnection(pika.ConnectionParameters(host='192.168.170.84'))
    channel = connection.channel()

    channel.queue_declare(queue='MQTT')

    def callback(ch, method, properties, body):
        #print(" [x] Received %r" % body)
        DB_Name =  "IoT.db"

        conn = mysql.connector.connect(host='192.168.170.85', user='root', password='IoTCloud_2019!', database='IoT')
        #Parse Data 
        
        
        json_Dict = json.loads(body)
        
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

    channel.basic_consume(queue='MQTT', on_message_callback=callback, auto_ack=True)

    #print(' [*] Waiting for messages. To exit press CTRL+C')
    channel.start_consuming()

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        #print('Interrupted')
        try:
            sys.exit(0)
        except SystemExit:
            os._exit(0)