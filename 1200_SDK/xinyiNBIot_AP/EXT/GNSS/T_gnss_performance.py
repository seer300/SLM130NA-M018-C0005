# -*- coding: utf-8 -*-
# @Time : 2023/1/9 14:11
# @Author : wangjian
# @Project : Python37


import sys
import time
import re
import operator
import random
import os
import serial
from inspect import stack
from datetime import datetime as dt
from pathlib import Path
from threading import currentThread
import itertools
import pandas as pd  # pip3 install pandas xlrd openpyxl
curPath = os.path.abspath(os.path.dirname(__file__))
rootPath = os.path.split(curPath)[0]
sys.path.append(os.path.split(curPath)[0])
from conf.conf_private import *
from lib.AT import ATOps
from lib.logview import LogView
import logging as log
from lib.ue_control import *
from lib.cmw500_control import *
from lib.excel_save import Excel_Save
from lib.V100B_control import *
log.basicConfig(filename="testcase.txt", level=log.DEBUG, format='%(asctime)s %(levelname)s: %(message)s',
                datefmt='%Y-%m-%d %H:%M:%S')
def get_frame_info(i):
    frame_info = stack()[i]
    file_name = os.path.split(frame_info.filename)[-1]
    line_num = frame_info.lineno
    tn = currentThread().getName()
    return f" {file_name} ({line_num})"
class gnss_performance(ATOps,V100B__Control):
    def __init__(self):
        pass

    def Relay(self, act, power_port):
        """
            act:{ON, OFF}
        """
        handler = serial.Serial('COM12', 9600, timeout=0.5,write_timeout=2)
        handler.flushInput()
        handler.write(f"AT+{act}".strip("\r\n").encode())
        time.sleep(0.5)
        handler.write(f"AT+STATE?".strip("\r\n").encode())
        while 1:
            res = handler.readline().decode(encoding='UTF-8',errors='ignore')
            if str(act) in str(res).split(':')[-1]:
                break            
        print(f"the state of relay is {act}")

    def cold_acq_sensitivity_Air780E(self):
        casename = sys._getframe().f_code.co_name
        for Rfpower in range(-130,-160,-1):
            V100B.set_GNSS_State("OFF")
            time.sleep(2)
            V100B.set_GNSS_State("On")
            time.sleep(10)
            V100B.set_GNSS_system_state(0, 1, 0, 0, 0)
            print(Rfpower)
            if Rfpower <= -130 and Rfpower >= -145:
                print('Rfpower1=',Rfpower)
                V100B.set_GNSS_Reference_Power(Rfpower)
                V100B.set_All_Gps_Power_Offset(11, 0)
                V100B.set_All_Bds_Power_Offset(11, 0)
                V100B.set_All_Gal_Power_Offset(6, 0)
            else:
                print('Rfpower2=', Rfpower)
                V100B.set_GNSS_Reference_Power(-145)
                V100B.set_All_Gps_Power_Offset(11, Rfpower-(-145))
                V100B.set_All_Bds_Power_Offset(11, Rfpower-(-145))
                V100B.set_All_Gal_Power_Offset(6, Rfpower-(-145))
            at.at_send("AT+CGNSPWR=0")
            time.sleep(1)
            at.at_send("AT+CGNSPWR=1")
            t1 = dt.now()
            print(t1)
            while 1:
                res = at.at_receive()
                print(res)
                if "RMC" in res:
                    if res.split(",")[3] and res.split(",")[2] == 'A':
                        t2 = dt.now()
                        TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                        print("TTFF=", TTFF)
                        pos_res = "success"
                        ES.get_excel_result(['casename', 'Rfpower', 'TTFF', 'pos_res'], [casename, Rfpower, TTFF, pos_res])
                        ES.save_to_excel(file_name=f"{casename}{'_BD.xlsx'}")
                        break
                if (dt.now()-t1).seconds > 600:
                        TTFF = round(((dt.now() - t1).seconds * 1000 + (dt.now() - t1).microseconds / 1000) / 1000, 3)
                        print("TTFF=", TTFF)
                        pos_res = "fail"
                        ES.get_excel_result(['casename', 'Rfpower', 'TTFF', 'pos_res'], [casename, Rfpower, TTFF, pos_res])
                        ES.save_to_excel(file_name=f"{casename}{'_BD.xlsx'}")
                        break

    def cold_acq_sen_HD8120(self):
        casename = sys._getframe().f_code.co_name
        for mode in ["GPS","BD"]:
            for Rfpower in range(-130,-160,-1):
                V100B.set_GNSS_State("OFF")
                time.sleep(3)
                V100B.set_GNSS_State("On")
                # V100B.set_GNSS_Reference_Power(-130)
                V100B.set_GNSS_system_state(1, 1, 0, 0, 0)
                V100B.set_All_Gps_Power_Offset(11, 0)
                V100B.set_All_Bds_Power_Offset(11, 0)
                V100B.set_All_Qzss_Power_Offset(193, 0)
                print(Rfpower)
                if Rfpower <= -130 and Rfpower >= -145:
                    print('Rfpower1=',Rfpower)
                    V100B.set_GNSS_Reference_Power(Rfpower)
                    V100B.set_All_Gps_Power_Offset(11, 0)
                    V100B.set_All_Bds_Power_Offset(11, 0)
                    V100B.set_All_Gal_Power_Offset(6, 0)
                else:
                    print('Rfpower2=', Rfpower)
                    V100B.set_GNSS_Reference_Power(-145)
                    V100B.set_All_Gps_Power_Offset(11, Rfpower-(-145))
                    V100B.set_All_Bds_Power_Offset(11, Rfpower-(-145))
                    V100B.set_All_Gal_Power_Offset(6, Rfpower-(-145))
                if "GPS" in mode:
                    while 1:
                        V100B.set_GNSS_system_state(1, 0, 0, 0, 0)
                        at.at_send_hex("F1 D9 06 0C 04 00 01 00 00 00 17 A0")  ##set GPS only mode
                        at.at_receive()
                        at.at_flush()
                        print(at.at_receive())
                        if "GPRMC" in at.at_receive():
                            print("set GPS mode success")
                            break
                elif "BD" in mode:
                    while 1:
                        V100B.set_GNSS_system_state(0, 1, 0, 0, 0)
                        at.at_send_hex("F1 D9 06 0C 04 00 04 00 00 00 1A AC")  ##set BD only mode
                        at.at_receive()
                        at.at_flush()
                        print(at.at_receive())
                        if "BDRMC" in at.at_receive():
                            print("set BDS mode success")
                            break
                at.at_send_hex("F1 D9 06 09 08 00 02 00 00 00 FF FF FF FF 15 01")  ##coldstart
                at.at_receive()                            
                t1 = dt.now()
                print(t1)
                while 1:
                    res = at.at_receive()
                    print(res)
                    if "RMC" in res:
                        if res.split(",")[3] and res.split(",")[2] == 'A':
                            t2 = dt.now()
                            TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                            print(f'{Rfpower}{"_coldstart_TTFF="}', TTFF)
                            pos_res = "success"
                            ES.get_excel_result(['casename', 'Rfpower', 'TTFF', 'pos_res'], [f"{casename}{'_'}{mode}", Rfpower, TTFF, pos_res])
                            ES.save_to_excel(file_name=f"{casename}{'.xlsx'}")
                            break
                    if (dt.now()-t1).seconds > 300:
                            TTFF = round(((dt.now() - t1).seconds * 1000 + (dt.now() - t1).microseconds / 1000) / 1000, 3)
                            print(f'{Rfpower}{"_coldstart_TTFF="}', TTFF)
                            pos_res = "fail"
                            ES.get_excel_result(['casename', 'Rfpower', 'TTFF', 'pos_res'], [f"{casename}{'_'}{mode}", Rfpower, TTFF, pos_res])
                            ES.save_to_excel(file_name=f"{casename}{'.xlsx'}")
                            break

                if pos_res == "fail":
                    break

    def cold_acq_sen_AT6558(self):
        casename = sys._getframe().f_code.co_name
        for mode in ["GNSS","GPS","BD"]:
            at.at_send('$PCAS10,3*1F')
            time.sleep(5)
            for Rfpower in range(-140,-160,-1):
                V100B.set_GNSS_State("OFF")
                time.sleep(3)
                V100B.set_GNSS_State("On")
                # V100B.set_GNSS_Reference_Power(-130)
                V100B.set_GNSS_system_state(1, 1, 0, 0, 0)
                V100B.set_All_Gps_Power_Offset(11, 0)
                V100B.set_All_Bds_Power_Offset(11, 0)
                V100B.set_All_Qzss_Power_Offset(193, 0)
                print(Rfpower)
                if Rfpower <= -130 and Rfpower >= -145:
                    print('Rfpower1=',Rfpower)
                    V100B.set_GNSS_Reference_Power(Rfpower)
                    V100B.set_All_Gps_Power_Offset(11, 0)
                    V100B.set_All_Bds_Power_Offset(11, 0)
                    V100B.set_All_Gal_Power_Offset(6, 0)
                else:
                    print('Rfpower2=', Rfpower)
                    V100B.set_GNSS_Reference_Power(-145)
                    V100B.set_All_Gps_Power_Offset(11, Rfpower-(-145))
                    V100B.set_All_Bds_Power_Offset(11, Rfpower-(-145))
                    V100B.set_All_Gal_Power_Offset(6, Rfpower-(-145))
                at.at_send("$PCAS10,2*1E")  ##coldstart
                at.at_receive()
                if "GPS" in mode:
                    while 1:
                        V100B.set_GNSS_system_state(1, 0, 0, 0, 0)
                        at.at_send("$PCAS04,1*18")  ##set GPS only mode
                        at.at_receive()
                        at.at_flush()
                        print(at.at_receive())
                        if "GPRMC" in at.at_receive():
                            print("set GPS mode success")
                            break
                elif "BD" in mode:
                    while 1:
                        V100B.set_GNSS_system_state(0, 1, 0, 0, 0)
                        at.at_send("$PCAS04,2*1B")  ##set BD only mode
                        at.at_receive()
                        at.at_flush()
                        print(at.at_receive())
                        if "BDRMC" in at.at_receive():
                            print("set BDS mode success")
                            break
                t1 = dt.now()
                print(t1)
                while 1:
                    res = at.at_receive()
                    print(res)
                    if "RMC" in res:
                        if res.split(",")[3] and res.split(",")[2] == 'A':
                            t2 = dt.now()
                            TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                            print(f'{Rfpower}{"_coldstart_TTFF="}', TTFF)
                            pos_res = "success"
                            ES.get_excel_result(['casename', 'Rfpower', 'TTFF', 'pos_res'], [f"{casename}{'_'}{mode}", Rfpower, TTFF, pos_res])
                            ES.save_to_excel(file_name=f"{casename}{'.xlsx'}")
                            break
                    if (dt.now()-t1).seconds > 300:
                            TTFF = round(((dt.now() - t1).seconds * 1000 + (dt.now() - t1).microseconds / 1000) / 1000, 3)
                            print(f'{Rfpower}{"_coldstart_TTFF="}', TTFF)
                            pos_res = "fail"
                            ES.get_excel_result(['casename', 'Rfpower', 'TTFF', 'pos_res'], [f"{casename}{'_'}{mode}", Rfpower, TTFF, pos_res])
                            ES.save_to_excel(file_name=f"{casename}{'.xlsx'}")
                            break

                if pos_res == "fail":
                    break

    def hot_acq_sen_AT6558(self):
        casename = sys._getframe().f_code.co_name
        at.at_flush()
        for mode in ["GNSS",'GPS','BD']:
            at.at_send('$PCAS10,3*1F')
            time.sleep(5)
            V100B.set_GNSS_State("OFF")
            time.sleep(3)
            V100B.set_GNSS_State("On")
            V100B.set_GNSS_Reference_Power(-130)
            V100B.set_GNSS_system_state(1, 1, 0, 0, 0)
            V100B.set_All_Gps_Power_Offset(11, 0)
            V100B.set_All_Bds_Power_Offset(11, 0)
            V100B.set_All_Qzss_Power_Offset(193, 0)
            if "GPS" in mode:
                while 1:
                    # V100B.set_GNSS_system_state(1, 0, 0, 0, 0)
                    at.at_send("$PCAS04,1*18")  ##set GPS only mode
                    at.at_receive()
                    at.at_flush()
                    if "GPRMC" in at.at_receive():
                        print("set GPS mode success")
                        break
            elif "BD" in mode:
                while 1:
                    # V100B.set_GNSS_system_state(0, 1, 0, 0, 0)
                    at.at_send("$PCAS04,2*1B")  ##set BD only mode
                    at.at_receive()
                    at.at_flush()
                    print(at.at_receive())
                    if "BDRMC" in at.at_receive():
                        print("set BDS mode success")
                        break
            at.at_send("$PCAS10,2*1E")##coldstart
            t1 = dt.now()
            at.at_flush()
            while 1:
                res = at.at_receive()
                print(res)
                if "RMC" in res:
                    if res.split(",")[3] and res.split(",")[2] == 'A':
                        print(res)
                if "GGA" in res:
                    if int(res.split(",")[7]) > 7:
                        break
            t2 = dt.now()
            TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
            print("coldstart TTFF=", TTFF)
            print('coldstart success')
            time.sleep(120)
            for Rfpower in range(-152,-165,-1):
                if Rfpower <= -130 and Rfpower >= -145:
                    print('Rfpower1=',Rfpower)
                    V100B.set_GNSS_Reference_Power(Rfpower)
                    V100B.set_All_Gps_Power_Offset(11, 0)
                    V100B.set_All_Bds_Power_Offset(11, 0)
                    V100B.set_All_Gal_Power_Offset(6, 0)
                else:
                    print('Rfpower2=', Rfpower)
                    V100B.set_GNSS_Reference_Power(-145)
                    V100B.set_All_Gps_Power_Offset(11, Rfpower-(-145))
                    V100B.set_All_Bds_Power_Offset(11, Rfpower-(-145))
                    V100B.set_All_Gal_Power_Offset(6, Rfpower-(-145))
                time.sleep(20)
                at.at_send("$PCAS10,0*1C")  ##hotstarta
                t1 = dt.now()
                
                print(t1)
                at.at_flush()
                time.sleep(5)
                while 1:
                    at.at_flush()
                    res = at.at_receive()
                    if "RMC" in res:
                        print(res)
                        if res.split(",")[3] and res.split(",")[2] == 'A':
                            
                            t2 = dt.now()
                            TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                            print(f'{Rfpower}{"_hotstart_TTFF="}', TTFF)
                            pos_res = "success"
                            ES.get_excel_result(['casename', 'Rfpower', 'TTFF', 'pos_res'], [f"{casename}{'_'}{mode}", Rfpower, TTFF, pos_res])
                            ES.save_to_excel(file_name=f"{casename}{'.xlsx'}")
                            break
                    if (dt.now()-t1).seconds > 300:
                            TTFF = round(((dt.now() - t1).seconds * 1000 + (dt.now() - t1).microseconds / 1000) / 1000, 3)
                            print(f'{Rfpower}{"_hotstart_TTFF="}', TTFF)
                            pos_res = "fail"
                            ES.get_excel_result(['casename', 'Rfpower', 'TTFF', 'pos_res'], [f"{casename}{'_'}{mode}", Rfpower, TTFF, pos_res])
                            ES.save_to_excel(file_name=f"{casename}{'.xlsx'}")
                            break

                if pos_res == "fail":
                    break
                # time.sleep(10)
                # V100B.set_GNSS_Reference_Power(-130)
                # V100B.set_GNSS_system_state(1, 1, 0, 0, 0)
                # V100B.set_All_Gps_Power_Offset(11, 0)
                # V100B.set_All_Bds_Power_Offset(11, 0)
                # V100B.set_All_Qzss_Power_Offset(193, 0)
                # at.at_send("$PCAS10,2*1E")##coldstart
                # t1 = dt.now()
                # at.at_flush()
                # while 1:
                    # res = at.at_receive()
                    # at.at_flush()
                    # print(res)
                    # if "RMC" in res:
                        # if res.split(",")[3] and res.split(",")[2] == 'A':
                            # print(res)
                    # if "GGA" in res:
                        # if int(res.split(",")[7]) > 7:
                            # break
                # t2 = dt.now()
                # TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                # print("coldstart TTFF=", TTFF)
                # print('coldstart success')
                # time.sleep(120)

    def hot_acq_sen_HD8120(self):
        casename = sys._getframe().f_code.co_name
        at.at_flush()
        # for mode in ["GNSS",'GPS','BD']:
        for i in range(1,3):
            for mode in ['GNSS']:
                at.at_send_hex('F1 D9 06 09 08 00 02 00 00 00 FF FF FF FF 15 01')
                time.sleep(5)
                V100B.set_GNSS_State("OFF")
                time.sleep(3)
                V100B.set_GNSS_State("On")
                V100B.set_GNSS_Reference_Power(-130)
                V100B.set_GNSS_system_state(1, 1, 0, 0, 0)
                V100B.set_All_Gps_Power_Offset(11, 0)
                V100B.set_All_Bds_Power_Offset(11, 0)
                if "GPS" in mode:
                    while 1:
                        # V100B.set_GNSS_system_state(1, 0, 0, 0, 0)
                        at.at_send_hex("F1 D9 06 0C 04 00 01 00 00 00 17 A0")  ##set GPS only mode
                        at.at_receive()
                        if "GPRMC" in at.at_receive():
                            print("set GPS mode success")
                            break
                elif "BD" in mode:
                    while 1:
                        # V100B.set_GNSS_system_state(0, 1, 0, 0, 0)
                        at.at_send_hex("F1 D9 06 0C 04 00 04 00 00 00 1A AC")  ##set BD only mode
                        at.at_receive()
                        print(at.at_receive())
                        if "BDRMC" in at.at_receive():
                            print("set BDS mode success")
                            break
                at.at_send_hex("F1 D9 06 40 01 00 01 48 22")##coldstart
                t1 = dt.now()
                at.at_flush()
                while 1:
                    res = at.at_receive()
                    at.at_flush()
                    if "RMC" in res:
                        if res.split(",")[3] and res.split(",")[2] == 'A':
                            print(res)
                    if "GGA" in res:
                        if int(res.split(",")[7]) > 7:
                            break
                t2 = dt.now()
                TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                print("coldstart TTFF=", TTFF)
                print('coldstart success')
                time.sleep(120)
                for Rfpower in range(-130,-165,-1):
                    if Rfpower <= -130 and Rfpower >= -145:
                        print('Rfpower1=',Rfpower)
                        V100B.set_GNSS_Reference_Power(Rfpower)
                        V100B.set_All_Gps_Power_Offset(11, 0)
                        V100B.set_All_Bds_Power_Offset(11, 0)
                    else:
                        print('Rfpower2=', Rfpower)
                        V100B.set_GNSS_Reference_Power(-145)
                        V100B.set_All_Gps_Power_Offset(11, Rfpower-(-145))
                        V100B.set_All_Bds_Power_Offset(11, Rfpower-(-145))
                    time.sleep(3)
                    at.at_send_hex("F1 D9 06 40 01 00 03 4A 24")  ##hotstart
                    t1 = dt.now()
                    print(t1)
                    at.at_flush()
                    time.sleep(5)                
                    while 1:
                        res = at.at_receive()
                        at.at_flush()
                        if "RMC" in res:
                            print(res)
                            if res.split(",")[3] and res.split(",")[2] == 'A':
                                t2 = dt.now()
                                TTFF = round((((t2 - t1).seconds) * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                                print(f'{Rfpower}{"_hotstart_TTFF="}', TTFF)
                                pos_res = "success"
                                ES.get_excel_result(['casename', 'Rfpower', 'TTFF', 'pos_res'], [f"{casename}{'_'}{mode}", Rfpower, TTFF, pos_res])
                                ES.save_to_excel(file_name=f"{casename}{'.xlsx'}")
                                break
                        if (dt.now()-t1).seconds > 600:
                                TTFF = round(((dt.now() - t1).seconds * 1000 + (dt.now() - t1).microseconds / 1000) / 1000, 3)
                                print(f'{Rfpower}{"_hotstart_TTFF="}', TTFF)
                                pos_res = "fail"
                                ES.get_excel_result(['casename', 'Rfpower', 'TTFF', 'pos_res'], [f"{casename}{'_'}{mode}", Rfpower, TTFF, pos_res])
                                ES.save_to_excel(file_name=f"{casename}{'.xlsx'}")
                                break
                        
                    if pos_res == "fail":
                        break
                # time.sleep(10)
                # V100B.set_GNSS_Reference_Power(-130)
                # V100B.set_GNSS_system_state(1, 1, 0, 0, 0)
                # V100B.set_All_Gps_Power_Offset(11, 0)
                # V100B.set_All_Bds_Power_Offset(11, 0)
                # at.at_send_hex("F1 D9 06 40 01 00 01 48 22")##coldstart
                # t1 = dt.now()
                # at.at_flush()
                # while 1:
                    # res = at.at_receive()
                    # at.at_flush()
                    # if "RMC" in res:
                        # print(res)
                        # if res.split(",")[3] and res.split(",")[2] == 'A':
                            # t2 = dt.now()
                            # TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                            # print("coldstart TTFF=", TTFF)
                    # if "GGA" in res:
                        # if int(res.split(",")[7]) > 7:
                            # break
                # print('coldstart success')
                    time.sleep(120)


    def hot_acq_sen_HD8120_loss_onepointtwo(self):
        casename = sys._getframe().f_code.co_name
        at.at_flush()
        # for mode in ["GNSS",'GPS','BD']:
        for i in range(1,3):
            for mode in ['GNSS']:
                at.at_send_hex('F1 D9 06 09 08 00 02 00 00 00 FF FF FF FF 15 01')
                time.sleep(5)
                V100B.set_GNSS_State("OFF")
                time.sleep(3)
                V100B.set_GNSS_State("On")
                V100B.set_GNSS_Reference_Power(-128.8)
                V100B.set_GNSS_system_state(1, 1, 0, 0, 0)
                V100B.set_All_Gps_Power_Offset(11, 0)
                V100B.set_All_Bds_Power_Offset(11, 0)
                if "GPS" in mode:
                    while 1:
                        # V100B.set_GNSS_system_state(1, 0, 0, 0, 0)
                        at.at_send_hex("F1 D9 06 0C 04 00 01 00 00 00 17 A0")  ##set GPS only mode
                        at.at_receive()
                        if "GPRMC" in at.at_receive():
                            print("set GPS mode success")
                            break
                elif "BD" in mode:
                    while 1:
                        # V100B.set_GNSS_system_state(0, 1, 0, 0, 0)
                        at.at_send_hex("F1 D9 06 0C 04 00 04 00 00 00 1A AC")  ##set BD only mode
                        at.at_receive()
                        print(at.at_receive())
                        if "BDRMC" in at.at_receive():
                            print("set BDS mode success")
                            break
                at.at_send_hex("F1 D9 06 40 01 00 01 48 22")##coldstart
                t1 = dt.now()
                at.at_flush()
                while 1:
                    res = at.at_receive()
                    at.at_flush()
                    if "RMC" in res:
                        if res.split(",")[3] and res.split(",")[2] == 'A':
                            print(res)
                    if "GGA" in res:
                        if int(res.split(",")[7]) > 7:
                            break
                t2 = dt.now()
                TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                print("coldstart TTFF=", TTFF)
                print('coldstart success')
                time.sleep(120)
                for Rfpower in range(int(-128.8),-165,-1):
                    if Rfpower <= -128.8 and Rfpower >= -143.8:
                        print('Rfpower1=',Rfpower)
                        V100B.set_GNSS_Reference_Power(Rfpower-0.8)
                        V100B.set_All_Gps_Power_Offset(11, 0)
                        V100B.set_All_Bds_Power_Offset(11, 0)
                    else:
                        print('Rfpower2=', Rfpower)
                        V100B.set_GNSS_Reference_Power(-143.8)
                        V100B.set_All_Gps_Power_Offset(11, Rfpower-(-143.8)-0.8)
                        V100B.set_All_Bds_Power_Offset(11, Rfpower-(-143.8)-0.8)
                    time.sleep(3)
                    at.at_send_hex("F1 D9 06 40 01 00 03 4A 24")  ##hotstart
                    t1 = dt.now()
                    print(t1)
                    at.at_flush()
                    time.sleep(5)                
                    while 1:
                        res = at.at_receive()
                        at.at_flush()
                        if "RMC" in res:
                            print(res)
                            if res.split(",")[3] and res.split(",")[2] == 'A':
                                t2 = dt.now()
                                TTFF = round((((t2 - t1).seconds) * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                                print(f'{Rfpower}{"_hotstart_TTFF="}', TTFF)
                                pos_res = "success"
                                ES.get_excel_result(['casename', 'Rfpower', 'TTFF', 'pos_res'], [f"{casename}{'_'}{mode}", Rfpower-0.8, TTFF, pos_res])
                                ES.save_to_excel(file_name=f"{casename}{'.xlsx'}")
                                break
                        if (dt.now()-t1).seconds > 600:
                                TTFF = round(((dt.now() - t1).seconds * 1000 + (dt.now() - t1).microseconds / 1000) / 1000, 3)
                                print(f'{Rfpower}{"_hotstart_TTFF="}', TTFF)
                                pos_res = "fail"
                                ES.get_excel_result(['casename', 'Rfpower', 'TTFF', 'pos_res'], [f"{casename}{'_'}{mode}", Rfpower-0.8, TTFF, pos_res])
                                ES.save_to_excel(file_name=f"{casename}{'.xlsx'}")
                                break
                        
                    if pos_res == "fail":
                        break
                # time.sleep(10)
                # V100B.set_GNSS_Reference_Power(-130)
                # V100B.set_GNSS_system_state(1, 1, 0, 0, 0)
                # V100B.set_All_Gps_Power_Offset(11, 0)
                # V100B.set_All_Bds_Power_Offset(11, 0)
                # at.at_send_hex("F1 D9 06 40 01 00 01 48 22")##coldstart
                # t1 = dt.now()
                # at.at_flush()
                # while 1:
                    # res = at.at_receive()
                    # at.at_flush()
                    # if "RMC" in res:
                        # print(res)
                        # if res.split(",")[3] and res.split(",")[2] == 'A':
                            # t2 = dt.now()
                            # TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                            # print("coldstart TTFF=", TTFF)
                    # if "GGA" in res:
                        # if int(res.split(",")[7]) > 7:
                            # break
                # print('coldstart success')
                    time.sleep(120)

    def hot_acq_sen_HD8120_loss_one(self):
        casename = sys._getframe().f_code.co_name
        at.at_flush()
        # for mode in ["GNSS",'GPS','BD']:
        for i in range(1,3):
            for mode in ['GNSS']:
                at.at_send_hex('F1 D9 06 09 08 00 02 00 00 00 FF FF FF FF 15 01')
                time.sleep(5)
                V100B.set_GNSS_State("OFF")
                time.sleep(3)
                V100B.set_GNSS_State("On")
                V100B.set_GNSS_Reference_Power(-129)
                V100B.set_GNSS_system_state(1, 1, 0, 0, 0)
                V100B.set_All_Gps_Power_Offset(11, 0)
                V100B.set_All_Bds_Power_Offset(11, 0)
                if "GPS" in mode:
                    while 1:
                        # V100B.set_GNSS_system_state(1, 0, 0, 0, 0)
                        at.at_send_hex("F1 D9 06 0C 04 00 01 00 00 00 17 A0")  ##set GPS only mode
                        at.at_receive()
                        if "GPRMC" in at.at_receive():
                            print("set GPS mode success")
                            break
                elif "BD" in mode:
                    while 1:
                        # V100B.set_GNSS_system_state(0, 1, 0, 0, 0)
                        at.at_send_hex("F1 D9 06 0C 04 00 04 00 00 00 1A AC")  ##set BD only mode
                        at.at_receive()
                        print(at.at_receive())
                        if "BDRMC" in at.at_receive():
                            print("set BDS mode success")
                            break
                at.at_send_hex("F1 D9 06 40 01 00 01 48 22")##coldstart
                t1 = dt.now()
                at.at_flush()
                while 1:
                    res = at.at_receive()
                    at.at_flush()
                    if "RMC" in res:
                        if res.split(",")[3] and res.split(",")[2] == 'A':
                            print(res)
                    if "GGA" in res:
                        if int(res.split(",")[7]) > 7:
                            break
                t2 = dt.now()
                TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                print("coldstart TTFF=", TTFF)
                print('coldstart success')
                time.sleep(120)
                for Rfpower in range(int(-129),-165,-1):
                    if Rfpower <= -129 and Rfpower >= -144:
                        print('Rfpower1=',Rfpower)
                        V100B.set_GNSS_Reference_Power(Rfpower)
                        V100B.set_All_Gps_Power_Offset(11, 0)
                        V100B.set_All_Bds_Power_Offset(11, 0)
                    else:
                        print('Rfpower2=', Rfpower)
                        V100B.set_GNSS_Reference_Power(-144)
                        V100B.set_All_Gps_Power_Offset(11, Rfpower-(-144))
                        V100B.set_All_Bds_Power_Offset(11, Rfpower-(-144))
                    time.sleep(3)
                    at.at_send_hex("F1 D9 06 40 01 00 03 4A 24")  ##hotstart
                    t1 = dt.now()
                    print(t1)
                    at.at_flush()
                    time.sleep(5)                
                    while 1:
                        res = at.at_receive()
                        at.at_flush()
                        if "RMC" in res:
                            print(res)
                            if res.split(",")[3] and res.split(",")[2] == 'A':
                                t2 = dt.now()
                                TTFF = round((((t2 - t1).seconds) * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                                print(f'{Rfpower}{"_hotstart_TTFF="}', TTFF)
                                pos_res = "success"
                                ES.get_excel_result(['casename', 'Rfpower', 'TTFF', 'pos_res'], [f"{casename}{'_'}{mode}", Rfpower, TTFF, pos_res])
                                ES.save_to_excel(file_name=f"{casename}{'.xlsx'}")
                                break
                        if (dt.now()-t1).seconds > 600:
                                TTFF = round(((dt.now() - t1).seconds * 1000 + (dt.now() - t1).microseconds / 1000) / 1000, 3)
                                print(f'{Rfpower}{"_hotstart_TTFF="}', TTFF)
                                pos_res = "fail"
                                ES.get_excel_result(['casename', 'Rfpower', 'TTFF', 'pos_res'], [f"{casename}{'_'}{mode}", Rfpower, TTFF, pos_res])
                                ES.save_to_excel(file_name=f"{casename}{'.xlsx'}")
                                break
                        
                    if pos_res == "fail":
                        break
                # time.sleep(10)
                # V100B.set_GNSS_Reference_Power(-130)
                # V100B.set_GNSS_system_state(1, 1, 0, 0, 0)
                # V100B.set_All_Gps_Power_Offset(11, 0)
                # V100B.set_All_Bds_Power_Offset(11, 0)
                # at.at_send_hex("F1 D9 06 40 01 00 01 48 22")##coldstart
                # t1 = dt.now()
                # at.at_flush()
                # while 1:
                    # res = at.at_receive()
                    # at.at_flush()
                    # if "RMC" in res:
                        # print(res)
                        # if res.split(",")[3] and res.split(",")[2] == 'A':
                            # t2 = dt.now()
                            # TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                            # print("coldstart TTFF=", TTFF)
                    # if "GGA" in res:
                        # if int(res.split(",")[7]) > 7:
                            # break
                # print('coldstart success')
                    time.sleep(120)

    def hot_acq_sen_HD8120_loss_zeropointseven(self):
        casename = sys._getframe().f_code.co_name
        at.at_flush()
        # for mode in ["GNSS",'GPS','BD']:
        for i in range(1,3):
            for mode in ['GNSS']:
                at.at_send_hex('F1 D9 06 09 08 00 02 00 00 00 FF FF FF FF 15 01')
                time.sleep(5)
                V100B.set_GNSS_State("OFF")
                time.sleep(3)
                V100B.set_GNSS_State("On")
                V100B.set_GNSS_Reference_Power(-129.3)
                V100B.set_GNSS_system_state(1, 1, 0, 0, 0)
                V100B.set_All_Gps_Power_Offset(11, 0)
                V100B.set_All_Bds_Power_Offset(11, 0)
                if "GPS" in mode:
                    while 1:
                        # V100B.set_GNSS_system_state(1, 0, 0, 0, 0)
                        at.at_send_hex("F1 D9 06 0C 04 00 01 00 00 00 17 A0")  ##set GPS only mode
                        at.at_receive()
                        if "GPRMC" in at.at_receive():
                            print("set GPS mode success")
                            break
                elif "BD" in mode:
                    while 1:
                        # V100B.set_GNSS_system_state(0, 1, 0, 0, 0)
                        at.at_send_hex("F1 D9 06 0C 04 00 04 00 00 00 1A AC")  ##set BD only mode
                        at.at_receive()
                        print(at.at_receive())
                        if "BDRMC" in at.at_receive():
                            print("set BDS mode success")
                            break
                at.at_send_hex("F1 D9 06 40 01 00 01 48 22")##coldstart
                t1 = dt.now()
                time.sleep(1)
                at.at_flush()
                while 1:
                    res = at.at_receive()
                    # at.at_flush()
                    if "RMC" in res:
                        if res.split(",")[3] and res.split(",")[2] == 'A':
                            print(res)
                    if "GGA" in res:
                        if int(res.split(",")[7]) > 7:
                            break
                t2 = dt.now()
                TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                print("coldstart TTFF=", TTFF)
                print('coldstart success')
                time.sleep(120)
                for Rfpower in range(int(-129.3),-165,-1):
                    if Rfpower <= -129 and Rfpower >= -144.3:
                        print('Rfpower1=',Rfpower)
                        V100B.set_GNSS_Reference_Power(Rfpower-0.3)
                        V100B.set_All_Gps_Power_Offset(11, 0)
                        V100B.set_All_Bds_Power_Offset(11, 0)
                    else:
                        print('Rfpower2=', Rfpower)
                        V100B.set_GNSS_Reference_Power(-144.3)
                        V100B.set_All_Gps_Power_Offset(11, Rfpower-(-144.3)-0.3)
                        V100B.set_All_Bds_Power_Offset(11, Rfpower-(-144.3)-0.3)
                    time.sleep(3)
                    at.at_send_hex("F1 D9 06 40 01 00 03 4A 24")  ##hotstart
                    t1 = dt.now()
                    print(t1)
                    at.at_flush()
                    time.sleep(5)                
                    while 1:
                        res = at.at_receive()
                        at.at_flush()
                        if "RMC" in res:
                            print(res)
                            if res.split(",")[3] and res.split(",")[2] == 'A':
                                t2 = dt.now()
                                TTFF = round((((t2 - t1).seconds) * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                                print(f'{Rfpower}{"_hotstart_TTFF="}', TTFF)
                                pos_res = "success"
                                ES.get_excel_result(['casename', 'Rfpower', 'TTFF', 'pos_res'], [f"{casename}{'_'}{mode}", Rfpower-0.3, TTFF, pos_res])
                                ES.save_to_excel(file_name=f"{casename}{'.xlsx'}")
                                break
                        if (dt.now()-t1).seconds > 600:
                                TTFF = round(((dt.now() - t1).seconds * 1000 + (dt.now() - t1).microseconds / 1000) / 1000, 3)
                                print(f'{Rfpower}{"_hotstart_TTFF="}', TTFF)
                                pos_res = "fail"
                                ES.get_excel_result(['casename', 'Rfpower', 'TTFF', 'pos_res'], [f"{casename}{'_'}{mode}", Rfpower, TTFF, pos_res])
                                ES.save_to_excel(file_name=f"{casename}{'.xlsx'}")
                                break
                        
                    if pos_res == "fail":
                        break
                # time.sleep(10)
                # V100B.set_GNSS_Reference_Power(-130)
                # V100B.set_GNSS_system_state(1, 1, 0, 0, 0)
                # V100B.set_All_Gps_Power_Offset(11, 0)
                # V100B.set_All_Bds_Power_Offset(11, 0)
                # at.at_send_hex("F1 D9 06 40 01 00 01 48 22")##coldstart
                # t1 = dt.now()
                # at.at_flush()
                # while 1:
                    # res = at.at_receive()
                    # at.at_flush()
                    # if "RMC" in res:
                        # print(res)
                        # if res.split(",")[3] and res.split(",")[2] == 'A':
                            # t2 = dt.now()
                            # TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                            # print("coldstart TTFF=", TTFF)
                    # if "GGA" in res:
                        # if int(res.split(",")[7]) > 7:
                            # break
                # print('coldstart success')
                    time.sleep(120)

    def cold_ttff_Air780E(self):
        casename = sys._getframe().f_code.co_name
        for Rfpower in range(-130,-150,-5):
            V100B.set_GNSS_State("OFF")
            time.sleep(10)
            V100B.set_GNSS_State("On")
            time.sleep(10)
            print(Rfpower)
            V100B.set_GNSS_Reference_Power(Rfpower)
            for i in range(0, 5):
                at.at_send("AT+CGNSPWR=0")
                time.sleep(1)
                at.at_send("AT+CGNSPWR=1")
                t1 = dt.now()
                print(t1)
                while 1:
                    res = at.at_receive()
                    print(res)
                    if "RMC" in res:
                        if res.split(",")[3] and res.split(",")[2] == 'A':
                            t2 = dt.now()
                            TTFF = round(((t2 - t1).seconds*1000+(t2-t1).microseconds/1000)/1000,3)
                            print("TTFF=", TTFF)
                            pos_res = "success"
                            ES.get_excel_result(['casename', 'Rfpower', 'TTFF', 'pos_res'],
                                                [casename, Rfpower, TTFF, pos_res])
                            ES.save_to_excel(file_name=f"{casename}{'.xlsx'}")
                            break
                    if (dt.now() - t1).seconds > 600:
                        TTFF = round(((dt.now() - t1).seconds * 1000 + (dt.now() - t1).microseconds / 1000) / 1000, 3)
                        print("TTFF=", TTFF)
                        pos_res = "fail"
                        ES.get_excel_result(['casename', 'Rfpower', 'TTFF', 'pos_res'],
                                            [casename, Rfpower, TTFF, pos_res])
                        ES.save_to_excel(file_name=f"{casename}{'.xlsx'}")
                        break

    def stop_then_start_ttff_EC800M(self):
        casename = sys._getframe().f_code.co_name
        V100B.V100B_reset()
        V100B.set_GNSS_system_state(1, 1, 1, 0, 1)
        V100B.set_GNSS_State("OFF")
        time.sleep(3)
        V100B.set_GNSS_State("On")
        time.sleep(5)
        at1.at_send("AT+CFUN=0")
        V100B.set_GNSS_Reference_Power(-130)
        for Rfpower in range(-130,-145,-5):
            print(Rfpower)
            V100B.set_GNSS_State("OFF")
            time.sleep(3)
            V100B.set_GNSS_State("On")
            time.sleep(5)
            V100B.set_GNSS_Reference_Power(Rfpower)
            for sleeptime in [30,60,300,600,900,1800,3600,7200,10800]:
                print(sleeptime)
                for i in range(0, 3):
                    at1.at_send("AT+QSCLK=1")
                    at1.at_send("AT+QGPS=0")
                    time.sleep(2)
                    at1.at_send("AT+QGPS=1")
                    while at1.wait_notify("GNSS Update successed"):
                        break
                    at1.at_send("AT+QGPSDEL=0")
                    at2.at_flush()
                    while 1:
                        res = at2.at_receive()
                        print(res)
                        if "RMC" in res:
                            if res.split(",")[3] and res.split(",")[2] == 'A':
                                print('pos success')
                        if "GGA" in res:
                            if int(res.split(",")[7]) > 8:
                                break
                    print('sleeptime_start=', dt.now())
                    at1.at_send("AT+QGPS=0")
                    time.sleep(sleeptime)
                    print('sleeptime_end=', dt.now())
                    at1.at_send("AT+QGPS=1")
                    while at1.wait_notify("GNSS Update successed"):
                        break
                    t1 = dt.now()
                    print('t1=',t1)
                    at2.at_flush()
                    while 1:
                        res = at2.at_receive()
                        print(res)
                        if "RMC" in res:
                            if res.split(",")[3] and res.split(",")[2] == 'A':
                                t2 = dt.now()
                                TTFF = round(((t2 - t1).seconds*1000+(t2-t1).microseconds/1000)/1000,3)
                                print("TTFF=", TTFF)
                                pos_res = "success"
                                ES.get_excel_result(['casename', 'Rfpower', 'TTFF', 'pos_res','sleeptime'],
                                                    [casename, Rfpower, TTFF, pos_res,sleeptime])
                                ES.save_to_excel(file_name=f"{casename}{'.xlsx'}")
                                break
                            if (dt.now() - t1).seconds > 1200:
                                TTFF = round(((dt.now() - t1).seconds * 1000 + (dt.now() - t1).microseconds / 1000) / 1000, 3)
                                print("TTFF=", TTFF)
                                pos_res = "fail"
                                ES.get_excel_result(['casename', 'Rfpower', 'TTFF', 'pos_res', 'sleeptime'],
                                                    [casename, Rfpower, TTFF, pos_res, sleeptime])
                                ES.save_to_excel(file_name=f"{casename}{'.xlsx'}")
                                break

    def reduce_GPS_SVs_number_M8B(self):
        casename = sys._getframe().f_code.co_name
        # V100B.V100B_reset()
        # V100B.set_GNSS_State("OFF")
        # time.sleep(3)
        # V100B.set_GNSS_State("On")
        # time.sleep(5)
        #
        # V100B.set_GNSS_Reference_Power(-130)
        # V100B.set_GNSS_system_state(1, 0, 0, 0, 0)
        # power.set_Voltage(1, 1.8)
        # power.set_state("reboot")
        # power.set_current_log_sample(300)
        # power.trig_dlog(f"{casename}{'.dlog'}")
        # time.sleep(310)
        # for rfpower in [ -137,-138,-139,-140,-141,-142,-143,-144,-145]:
        for offset in range(-1,-10,-1):
            V100B.set_All_Gps_Power_Offset(1, offset)
            V100B.set_GNSS_Reference_Power(-145)
            time.sleep(10)
            for sv_id in [1,3,6,11,14,19,20,22,27,28,32]:
                print(sv_id)
                V100B.set_SV_state(sv_id,"GPS",0)
                SVs = V100B.get_number_of_active_SVs("GPS").rstrip('\r\n')
                power.set_current_log_sample(40)
                power.trig_dlog(f"{-145+offset}{'_'}{SVs}{'_GPS_M8B_dBm.dlog'}")
                time.sleep(42)
            for sv_id in [1, 3, 6, 11, 14, 19, 20, 22, 27, 28, 32]:
                print(sv_id)
                V100B.set_SV_state(sv_id, "GPS", 1)

    def coldstart_EC800M(self):
        casename = sys._getframe().f_code.co_name
        for i in range(1, 5):
            V100B.set_GNSS_State("OFF")
            time.sleep(3)
            V100B.set_GNSS_State("On")
            at1.at_send("AT+QGPS=0")
            time.sleep(2)
            at1.at_send("AT+QGPS=1")
            while at1.wait_notify("GNSS Update successed"):
                break
            at1.at_send("AT+QGPSDEL=0")
            at2.at_flush()
            t1=dt.now()
            print(t1)
            while 1:
                res = at2.at_receive()
                print(res)
                if "RMC" in res:
                    if res.split(",")[3] and res.split(",")[2] == 'A':
                        t2 = dt.now()
                        print(t2)
                        TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                        print("TTFF=", TTFF)
                        break

    def coldstart_TTFF_AT6558(self):
        casename = sys._getframe().f_code.co_name
        at.at_send('$PCAS10,3*1F')
        time.sleep(5)
        V100B.set_GNSS_State("OFF")
        time.sleep(3)
        V100B.set_GNSS_State("On")
        V100B.set_GNSS_Reference_Power(-130)
        V100B.set_GNSS_system_state(1, 1, 0, 0, 0)
        V100B.set_All_Gps_Power_Offset(11, 0)
        V100B.set_All_Bds_Power_Offset(11, 0)
        V100B.set_All_Qzss_Power_Offset(193, 0)
        for waittime in range(30,49,1):
            for i in range(1, 10):
                at.at_send("$PCAS10,2*1E")##coldstart
                at.at_flush()
                t1=dt.now()
                print(t1)
                while 1:
                    res = at.at_receive()
                    at.at_flush()
                    if "RMC" in res:
                        print(res)
                        if res.split(",")[3] and res.split(",")[2] == 'A':
                            t2 = dt.now()
                            print(t2)
                            TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                            print("TTFF=", TTFF)
                            ES.get_excel_result(['casename','test_time', 'TTFF', 'waittime'],
                                                [f'{casename}{"_"}{mode}',i, TTFF, waittime])
                            ES.save_to_excel(file_name=f'{casename}{".xlsx"}')                            
                            break
                time.sleep(waittime)


    def coldstart_TTFF_AT6558_power(self):
        casename = sys._getframe().f_code.co_name
        pos1 = (2020,8,31,9,12,0,'"Tokyo"')
        pos2 = (2021,3,19,15,36,0,'"New Delhi"')
        pos3 = (2023,10,27,22,55,0,'"Seoul"')
        pos_list = [pos1,pos2,pos3]
        # at.at_send('$PCAS10,3*1F')
        # time.sleep(5)
        for i in range(1, 20):
            wait = random.randint(3,5)
            V100B.set_GNSS_State("OFF")
            time.sleep(3)
            pos = random.choice(pos_list)
            print(pos)
            V100B.set_initial_state(*pos)
            V100B.set_GNSS_State("On")
            V100B.set_GNSS_Reference_Power(-126.3)
            V100B.set_GNSS_system_state(1, 1, 0, 0, 0)
            V100B.set_All_Gps_Power_Offset(11, 0)
            V100B.set_All_Bds_Power_Offset(11, 0)
            
            self.Relay('OFF','COM12')##power off
            off_on_wait = random.randint(10,40)
            # off_on_wait = 5
            time.sleep(off_on_wait)
            self.Relay('ON','COM12')##power on
            t1=dt.now()
            print("off_on_wait=",off_on_wait)
            print(i)
            print(t1)
            time.sleep(15)
            at.at_open("COM17", 9600)
            mode = "GPS+BD"
            at.at_send('$PCAS04,3*1A')
            at.at_send('$PCAS03,0,0,0,0,1,0,0,0,0,0,,,0,0*03')
            time.sleep(2)
            at.at_flush()
            time.sleep(1)
            while 1:
                # at.at_flush()
                res = at.at_receive()
                if "RMC" in res or "$" in res:
                    print(at.nowtime,res)
                    if res.split(",")[3] and res.split(",")[2] == 'A':
                        # t2 = dt.now()
                        TTFF = round(((at.nowtime - t1).seconds * 1000 + (at.nowtime - t1).microseconds / 1000) / 1000, 3)
                        # TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                        print("TTFF=", TTFF)
                        # if int(res.split(",")[1].split(".")[1])>500:
                            # gps_time_second = int(str(res.split(",")[1].split(".")[0])[4:6])+1
                        # else:
                            # gps_time_second = int(str(res.split(",")[1].split(".")[0])[4:6])
                        # print('gps_time_second=',gps_time_second)
                        # wait = (81-gps_time_second)%30
                        # random.seed(gps_time_second)
                        # wait_time = random.randint(wait-1,wait+6)
                        time.sleep(wait)
                        print('wait_time=',wait)
                        ES.get_excel_result(['casename','test_time', 'TTFF', 'waittime','off_on_wait','pos'],
                                            [f'{casename}{"_"}{mode}',i, TTFF, wait,off_on_wait,pos])
                        ES.save_to_excel(file_name=f'{casename}{".xlsx"}')
                       
                        break
                if (dt.now() - t1).seconds >= 60:
                    ES.get_excel_result(['casename','test_time', 'TTFF', 'waittime','pos'],
                                            [f'{casename}{"_"}{mode}',i, 60, wait,pos])
                    ES.save_to_excel(file_name=f'{casename}{".xlsx"}')
                    break                    
            at.at_close()
            


              
    def coldstart_TTFF_HD8120(self):
        casename = sys._getframe().f_code.co_name
        at.at_send_hex('F1 D9 06 09 08 00 02 00 00 00 FF FF FF FF 15 01')
        time.sleep(5)
        V100B.set_GNSS_State("OFF")
        time.sleep(3)
        V100B.set_GNSS_State("On")
        V100B.set_GNSS_Reference_Power(-127)
        V100B.set_GNSS_system_state(1, 1, 0, 0, 0)
        V100B.set_All_Gps_Power_Offset(11, 0)
        V100B.set_All_Bds_Power_Offset(11, 0)
        for waittime in range(30,49,1):
            for i in range(1, 50):
                at.at_send_hex('F1 D9 06 09 08 00 02 00 00 00 FF FF FF FF 15 01')##coldstart
                at.at_flush()
                t1=dt.now()
                print(t1)
                while 1:
                    res = at.at_receive()
                    at.at_flush()
                    if "RMC" in res:
                        print(res)
                        if res.split(",")[3] and res.split(",")[2] == 'A':
                            t2 = dt.now()
                            print(t2)
                            TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                            print("TTFF=", TTFF)
                            ES.get_excel_result(['casename','test_time', 'TTFF', 'waittime'],
                                                [f'{casename}',i, TTFF, waittime])
                            ES.save_to_excel(file_name=f'{casename}{".xlsx"}')                            
                            break
                time.sleep(waittime)
    def coldstart_NK6050(self):
        casename = sys._getframe().f_code.co_name
        for i in range(1, 5):
            time.sleep(random.randint(0,30))
            V100B.set_GNSS_State("OFF")
            time.sleep(3)
            V100B.set_GNSS_State("On")
            # at1.at_send("AT+NREBOOT")
            at1.at_send("AT+NREBOOT")
            # while at1.wait_notify("NBOOTMODE: 0,1"):
            #     break
            time.sleep(1)
            at1.at_send("AT+NENSLEEP=0")
            at1.at_send("AT+NENSLEEP=0")
            while at1.wait_notify("OK"):
                break
            time.sleep(1)
            at1.at_send("AT+NGTRACK=1")
            while at1.wait_notify("OK"):
                break
            at1.at_flush()
            t1=dt.now()
            print(t1)
            while 1:
                res = at1.at_receive()
                print(res)
                if "RMC" in res:
                    if res.split(",")[3] and res.split(",")[2] == 'A':
                        t2 = dt.now()
                        print(t2)
                        TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                        print("TTFF=", TTFF)
                        break

    def tracking_sen_AT6558(self):
        casename = sys._getframe().f_code.co_name

        for mode in ["GNSS","GPS","BD"]:
            for i in range(1,2):
                V100B.set_GNSS_State("OFF")
                time.sleep(3)
                V100B.set_GNSS_State("On")
                V100B.set_GNSS_Reference_Power(-130)
                V100B.set_GNSS_system_state(1, 1, 0, 0, 0)
                if "GPS" in mode:
                    V100B.set_GNSS_system_state(1, 0, 0, 0, 0)
                elif "BD" in mode:
                    V100B.set_GNSS_system_state(0, 1, 0, 0, 0)
                V100B.set_All_Gps_Power_Offset(11, 0)
                V100B.set_All_Bds_Power_Offset(11, 0)
                V100B.set_All_Qzss_Power_Offset(193, 0)
                at.at_send("$PCAS10,2*1E")
                at.at_flush()
                t1=dt.now()
                print(t1)
                while 1:
                    res = at.at_receive()
                    # print(res)
                    if "RMC" in res:
                        if res.split(",")[3] and res.split(",")[2] == 'A':
                            t2 = dt.now()
                            print(t2)
                            TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                            print("coldstart TTFF=", TTFF)
                    if "GGA" in res:
                        if int(res.split(",")[7]) > 7:
                            break
                V100B.set_GNSS_Reference_Power(-145)
                at.at_flush()
                t1=dt.now()
                print(t1)
                while 1:
                    res = at.at_receive()
                    at.at_flush()
                    # print(res)
                    if "RMC" in res:
                        if res.split(",")[3] and res.split(",")[2] == 'A':
                            t2 = dt.now()
                            break
                for offset in range(-5,-20,-1):
                    print(offset)
                    V100B.set_All_Gps_Power_Offset(11, offset)
                    V100B.set_All_Bds_Power_Offset(11, offset)
                    V100B.set_All_Qzss_Power_Offset(193, offset)
                    time.sleep(20)
                    at.at_flush()
                    t1=dt.now()
                    print(t1)
                    while 1:
                        res = at.at_receive()
                        at.at_flush()
                        print(res)
                        if "RMC" in res:
                            if res.split(",")[3] and res.split(",")[2] == 'A':
                                tracking_pos_res = "success"
                                ES.get_excel_result(['casename','test_time', 'Rfpower', 'tracking_pos_res'],
                                                    [f'{casename}{"_"}{mode}',i, -145+offset, tracking_pos_res])
                                ES.save_to_excel(file_name=f'{casename}{".xlsx"}')
                            # elif res.split(",")[2] == 'V':
                                # tracking_pos_res = "fail"
                                # ES.get_excel_result(['casename','test_time', 'Rfpower', 'tracking_pos_res'],
                                                    # [f'{casename}{"_"}{mode}',i, -145 + offset, tracking_pos_res])
                                # ES.save_to_excel(file_name=f'{casename}{".xlsx"}')
                            break
                        if (dt.now() - t1).seconds > 30:
                            tracking_pos_res = "fail"
                            ES.get_excel_result(['casename','test_time', 'Rfpower', 'tracking_pos_res'],
                                                [f'{casename}{"_"}{mode}',i, -145 + offset, tracking_pos_res])
                            ES.save_to_excel(file_name=f'{casename}{".xlsx"}')                            
                            break
                    if tracking_pos_res == "fail":
                        break
    def tracking_sen_HD8120(self):
        casename = sys._getframe().f_code.co_name
        for mode in ["BD"]:
            for i in range(1, 3):
                V100B.set_GNSS_State("OFF")
                time.sleep(3)
                V100B.set_GNSS_State("On")
                V100B.set_GNSS_Reference_Power(-130)
                V100B.set_GNSS_system_state(1, 1, 0, 0, 0)
                V100B.set_All_Gps_Power_Offset(11, 0)
                V100B.set_All_Bds_Power_Offset(11, 0)
                V100B.set_All_Qzss_Power_Offset(193, 0)
                if "GPS" in mode:
                    while 1:
                        V100B.set_GNSS_system_state(1, 0, 0, 0, 0)
                        at.at_send_hex("F1 D9 06 0C 04 00 01 00 00 00 17 A0")
                        at.at_receive()
                        at.at_flush()
                        print(at.at_receive())
                        if "GPRMC" in at.at_receive():
                            print("set GPS mode success")  ##set GPS only mode
                            break
                elif "BD" in mode:
                    while 1:
                        V100B.set_GNSS_system_state(0, 1, 0, 0, 0)
                        at.at_send_hex("F1 D9 06 0C 04 00 04 00 00 00 1A AC")  ##set BD only mode
                        at.at_receive()
                        at.at_flush()
                        print(at.at_receive())
                        if "BDRMC" in at.at_receive():
                            print("set BD mode success")
                            break
                at.at_flush()
                t1=dt.now()
                at.at_send_hex("F1 D9 06 09 08 00 02 00 00 00 FF FF FF FF 15 01")
                at.at_receive()                
                print(t1)
                while 1:
                    res = at.at_receive()
                    # print(res)
                    if "RMC" in res:
                        if res.split(",")[3] and res.split(",")[2] == 'A':
                            t2 = dt.now()
                            print(t2)
                            TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                            print("coldstart TTFF=", TTFF)
                    if "GGA" in res:
                        if int(res.split(",")[7]) > 8:
                            break
                for Rfpower in range(-130,-160,-1):
                    if Rfpower <= -130 and Rfpower >= -145:
                        print('Rfpower1=',Rfpower)
                        V100B.set_GNSS_Reference_Power(Rfpower)
                        V100B.set_All_Gps_Power_Offset(11, 0)
                        V100B.set_All_Bds_Power_Offset(11, 0)
                        V100B.set_All_Gal_Power_Offset(6, 0)
                    else:
                        print('Rfpower2=', Rfpower)
                        V100B.set_GNSS_Reference_Power(-145)
                        V100B.set_All_Gps_Power_Offset(11, Rfpower-(-145))
                        V100B.set_All_Bds_Power_Offset(11, Rfpower-(-145))
                        V100B.set_All_Gal_Power_Offset(6, Rfpower-(-145))
                    at.at_flush()
                    t1=dt.now()
                    print(t1)
                    while 1:
                        res = at.at_receive()
                        print(res)
                        if "RMC" in res:
                            if res.split(",")[3] and res.split(",")[2] == 'A':
                                tracking_pos_res = "success"
                                ES.get_excel_result(['casename','test_time', 'Rfpower', 'tracking_pos_res'],
                                                    [f'{casename}{"_"}{mode}',i, Rfpower, tracking_pos_res])
                                ES.save_to_excel(file_name=f'{casename}{".xlsx"}')
                            elif res.split(",")[2] == 'V':
                                tracking_pos_res = "fail"
                                ES.get_excel_result(['casename','test_time', 'Rfpower', 'tracking_pos_res'],
                                                    [f'{casename}{"_"}{mode}',i, Rfpower, tracking_pos_res])
                                ES.save_to_excel(file_name=f'{casename}{".xlsx"}')
                            break
                        if (dt.now() - t1).seconds > 120:
                            break

    def reacq_sen_AT6558(self):
        casename = sys._getframe().f_code.co_name
        # for mode in ["GPS","BD"]:
        for mode in ["GNSS","GPS","BD"]:
            at.at_send('$PCAS10,3*1F')
            time.sleep(5)
            print(mode)
            for offset in range(-20,0,1):
                print(offset)
                V100B.set_GNSS_State("OFF")
                time.sleep(3)
                V100B.set_GNSS_State("On")
                V100B.set_GNSS_Reference_Power(-130)
                V100B.set_GNSS_system_state(1, 1, 0, 0, 0)
                V100B.set_All_Gps_Power_Offset(11, 0)
                V100B.set_All_Bds_Power_Offset(11, 0)
                V100B.set_All_Qzss_Power_Offset(193, 0)
                at.at_send("$PCAS10,2*1E")##coldstart
                at.at_receive()
                if "GPS" in mode:
                    print('gggggggggggggggggggggggggg')
                    while 1:
                        # V100B.set_GNSS_system_state(1, 0, 0, 0, 0)
                        at.at_send("$PCAS04,1*18")  ##set GPS only mode
                        at.at_receive()
                        if "GPRMC" in at.at_receive():
                            print("set GPS mode success")
                            break
                elif "BD" in mode:
                    print('bbbbbbbbbbbbbbbbbbbbbbbbbbbb')
                    while 1:
                        # V100B.set_GNSS_system_state(0, 1, 0, 0, 0)
                        at.at_send("$PCAS04,2*1B")  ##set BD only mode
                        at.at_receive()
                        print(at.at_receive())
                        if "BDRMC" in at.at_receive():
                            print("set BDS mode success")
                            break
                at.at_flush()
                t1=dt.now()
                print(t1)
                while 1:
                    res = at.at_receive()
                    at.at_flush()
                    if "RMC" in res:
                        print(res)
                        if res.split(",")[3] and res.split(",")[2] == 'A':
                            print(res)

                    if "GGA" in res:
                        print(res)
                        if int(res.split(",")[7]) > 7:
                            break
                t2 = dt.now()
                print(t2)
                TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                print("coldstart TTFF=", TTFF)                            
                time.sleep(20)##定位成功后再持续定位2分钟
                V100B.set_RF_State("OFF")##关闭RF信号
                time.sleep(30)##关闭信号30s
                V100B.set_GNSS_Reference_Power(-145)
                print(dt.now())
                V100B.set_All_Gps_Power_Offset(11, offset)
                V100B.set_All_Bds_Power_Offset(11, offset)
                V100B.set_All_Qzss_Power_Offset(193, offset)
                V100B.set_RF_State("On")
                print(-145+offset)
                at.at_flush()
                t1=dt.now()
                print(t1)
                while 1:
                    at.at_flush()
                    res = at.at_receive()
                    if "RMC" in res:
                        print(res)
                        if res.split(",")[3] and res.split(",")[2] == 'A':
                            print(res)
                            reacq_pos_res = "success"
                            ES.get_excel_result(['casename', 'spend_time','Rfpower', 'reacq_pos_res'],
                                                [f"{casename}{'_'}{mode}{'.xlsx'}",(dt.now() - t1).seconds, -145+offset, reacq_pos_res])
                            ES.save_to_excel(file_name=f"{casename}{'.xlsx'}")
                            break
                    if (dt.now() - t1).seconds >= 60:
                        reacq_pos_res = "fail"
                        ES.get_excel_result(['casename', 'spend_time','Rfpower', 'reacq_pos_res'],
                                            [f"{casename}{'_'}{mode}{'.xlsx'}",(dt.now() - t1).seconds, -145 + offset, reacq_pos_res])
                        ES.save_to_excel(file_name=f"{casename}{'.xlsx'}")
                        break
                if reacq_pos_res == "success":
                    break

    def reacq_sen_HD8120(self):
        casename = sys._getframe().f_code.co_name
        # for mode in ["GPS","BD"]:
        for mode in ["GNSS","GPS","BD"]:
            at.at_send_hex('F1 D9 06 09 08 00 02 00 00 00 FF FF FF FF 15 01')
            time.sleep(5)
            print(mode)
            for offset in range(-15,0,1):
                print(offset)
                V100B.set_GNSS_State("OFF")
                time.sleep(3)
                V100B.set_GNSS_State("On")
                V100B.set_GNSS_Reference_Power(-130)
                V100B.set_GNSS_system_state(1, 1, 0, 0, 0)
                V100B.set_All_Gps_Power_Offset(11, 0)
                V100B.set_All_Bds_Power_Offset(11, 0)
                at.at_send_hex("F1 D9 06 09 08 00 02 00 00 00 FF FF FF FF 15 01")##coldstart
                at.at_receive()
                if "GPS" in mode:
                    print('gggggggggggggggggggggggggg')
                    while 1:
                        # V100B.set_GNSS_system_state(1, 0, 0, 0, 0)
                        at.at_send_hex("F1 D9 06 0C 04 00 01 00 00 00 17 A0")  ##set GPS only mode
                        at.at_receive()
                        if "GPRMC" in at.at_receive():
                            print("set GPS mode success")
                            break
                elif "BD" in mode:
                    print('bbbbbbbbbbbbbbbbbbbbbbbbbbbb')
                    while 1:
                        # V100B.set_GNSS_system_state(0, 1, 0, 0, 0)
                        at.at_send_hex("F1 D9 06 0C 04 00 04 00 00 00 1A AC")  ##set BD only mode
                        at.at_receive()
                        print(at.at_receive())
                        if "BDRMC" in at.at_receive():
                            print("set BDS mode success")
                            break
                at.at_flush()
                t1=dt.now()
                print(t1)
                while 1:
                    res = at.at_receive()
                    at.at_flush()
                    if "RMC" in res:
                        print(res)
                        if res.split(",")[3] and res.split(",")[2] == 'A':
                            print(res)
                    if "GGA" in res:
                        print(res)
                        if int(res.split(",")[7]) > 7:
                            break
                t2 = dt.now()
                print(t2)
                TTFF = round(((t2 - t1).seconds * 1000 + (t2 - t1).microseconds / 1000) / 1000, 3)
                print("coldstart TTFF=", TTFF)                            
                time.sleep(120)##定位成功后再持续定位2分钟
                V100B.set_RF_State("OFF")##关闭RF信号
                print('RF OFF')
                time.sleep(30)##关闭信号30s
                print('RF ON')
                V100B.set_RF_State("ON")##打开RF信号
                V100B.set_GNSS_Reference_Power(-145)
                print(dt.now())
                V100B.set_All_Gps_Power_Offset(11, offset)
                V100B.set_All_Bds_Power_Offset(11, offset)
                print(-145+offset)
                at.at_flush()
                t1=dt.now()
                print(t1)
                time.sleep(5)
                while 1:
                    res = at.at_receive()
                    at.at_flush()
                    if "RMC" in res:
                        print(res)
                        if res.split(",")[3] and res.split(",")[2] == 'A':
                            print(res)
                            reacq_pos_res = "success"
                            ES.get_excel_result(['casename', 'spend_time','Rfpower', 'reacq_pos_res'],
                                                [f"{casename}{'_'}{mode}{'.xlsx'}",(dt.now() - t1).seconds-5, -145+offset, reacq_pos_res])
                            ES.save_to_excel(file_name=f"{casename}{'.xlsx'}")
                            break
                    if (dt.now() - t1).seconds-5 >= 60:
                        reacq_pos_res = "fail"
                        ES.get_excel_result(['casename', 'spend_time','Rfpower', 'reacq_pos_res'],
                                            [f"{casename}{'_'}{mode}{'.xlsx'}",(dt.now() - t1).seconds-5, -145 + offset, reacq_pos_res])
                        ES.save_to_excel(file_name=f"{casename}{'.xlsx'}")
                        break
                if reacq_pos_res == "success":
                    break
if __name__ == "__main__":
    T = gnss_performance()
    V100B = V100B__Control(V100B_ip)
    # N6705C_USB_addr = 'USB0::0x2A8D::0x0F02::MY56002932::0::INSTR'
    # power = N6705C_Control(N6705C_USB_addr)
    at = ATOps()
    # at1 = ATOps()
    # at2= ATOps()
    ES = Excel_Save()
    at.at_open("COM19", 9600)
    # at1.at_open("COM13",115200)
    # at2.at_open("COM55",115200)
    # T.reduce_GPS_SVs_number_M8B()
    # T.stop_then_start_ttff_EC800M()
    # T.cold_acq_sensitivity_Air780E()
    # T.hot_acq_sen_AT6558()
    # T.hot_acq_sen_HD8120_loss_zeropointseven()
    # T.hot_acq_sen_HD8120_loss_one()
    T.coldstart_TTFF_AT6558_power()
    # T.hot_acq_sen_HD8120_loss_onepointtwo()
    # T.coldstart_TTFF_HD8120()
    # T.tracking_sen_HD8120()

