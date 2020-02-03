import paho.mqtt.client as mqtt
import json
from tkinter import *
import tkinter.filedialog
from PIL import Image, ImageTk
import json
import csv
import datetime


MQTTHOST = "uudp5h2.mqtt.iot.gz.baidubce.com"
MQTTPORT = 1883
MQTTNAME = "uudp5h2/iot_light_mqttfx_baidu_jx"
MQTTPASW = "bmCyRZ07ne2QZXvW"
visization_host = 'https://viz.baidubce.com/prod/sharing/dashboard/f56c5456a2ca8e9e3fb5136e1d8c826f'

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    client.subscribe("SW_LED")
    client.subscribe("JSON_ROAD")

#    client.publish("SW_LED", json.dumps({"user": user, "say": "Hello,anyone!"}))


def on_message(client, userdata, msg):
#    print(msg.topic+":"+str(msg.payload.decode()))
    print(msg.topic+":"+msg.payload.decode())
    if(msg.topic == 'SW_LED'):
        if(msg.payload.decode()=='warning'):
            lb_bazzer.configure(image=Img_bazzer_on)
        if(msg.payload.decode()=='warning_cancel'):
            lb_bazzer.configure(image=Img_bazzer_off)
		
    if(msg.topic == 'JSON_ROAD'):
        d = json.loads(msg.payload.decode())
        txt_Temp.configure(text=str(d['reported']['Temp'])+'℃')
        txt_Humid.configure(text=str(d['reported']['Humid'])+'%RH')
        txt_CO2.configure(text=str(d['reported']['CO2'])+'ppm')
        txt_MHMH.configure(text=str(d['reported']['MHMH']))
        txt_light.configure(text=str(d['reported']['light']))
        csv_writer.writerow([str(datetime.datetime.now())[0:19],
                            str(d['reported']['Temp']),
                            str(d['reported']['Humid']),
                            str(d['reported']['CO2']),
                            str(d['reported']['MHMH']),
                            str(d['reported']['light'])])
    txt.configure(text=msg.topic+":"+msg.payload.decode())
    payload = json.loads(msg.payload.decode())
    print(payload.get("user")+":"+payload.get("say"))

def publish_data():
    str = inp1.get()
    if str:
        client.publish("SW_LED", json.dumps({"user": user, "say": str}))

def Mysel():
    if(var.get() == 1):
        client.publish("SW_LED", "AUTO_ALL")
        btn_csent.configure(state='normal')
        btn_mwater.configure(state='disable')
        btn_mwind.configure(state='disable')
        btn_cmled_up.configure(state='disable')
        btn_cmled_down.configure(state='disable')
    else:
        client.publish("SW_LED", "MAN_ALL")
        btn_csent.configure(state='disable')
        btn_mwater.configure(state='normal')
        btn_mwind.configure(state='normal')
        btn_cmled_up.configure(state='normal')
        btn_cmled_down.configure(state='normal')
		
def warning_choose():
    if (warningVar1.get() == 0):
        print("no warning")
        lb_bazzer.configure(image = Img_bazzer_off)
        client.publish("SW_LED","warning_off")
    if (warningVar1.get() == 1):
        print("warning")
        lb_bazzer.configure(image = Img_bazzer_on)
        client.publish("SW_LED","warning_on")
	
def push_control():
    clight = 200
    cTemp = 28
    cMHMH = 75  
    txt_csent.configure(text='发送失败')
    if(int(inp_clight.get())>99 and int(inp_clight.get())<1000):
        clight = int(inp_clight.get())     
    if(int(inp_cTemp.get())>9 and int(inp_cTemp.get())<100):
        cTemp = int(inp_cTemp.get())
    if(int(inp_cMHMH.get())>9 and int(inp_cMHMH.get())<100):
        cMHMH = int(inp_cMHMH.get())
    client.publish("SW_LED", "{light:%d,Temp:%d,MHMH:%d}"%(clight,cTemp,cMHMH))
    txt_csent.configure(text='发送成功')

def push_warning():
    wlight = 200
    wTemp = 28
    wMHMH = 75  
    wHumid = 40
    wCO2 = 401
    txt_wsent.configure(text='发送失败')
    if(int(inp_wlight.get())>99 and int(inp_wlight.get())<1000):
        wlight = int(inp_wlight.get())     
    if(int(inp_wTemp.get())>9 and int(inp_wTemp.get())<100):
        wTemp = int(inp_wTemp.get())
    if(int(inp_wMHMH.get())>9 and int(inp_wMHMH.get())<100):
        wMHMH = int(inp_wMHMH.get())
    if(int(inp_wHumid.get())>9 and int(inp_wHumid.get())<100):
        wTemp = int(inp_wHumid.get())
    if(int(inp_wCO2.get())>99 and int(inp_wCO2.get())<1000):
        wMHMH = int(inp_wCO2.get())
    client.publish("SW_LED", "{light:%d,Temp:%d,MHMH:%d,Humid:%d,CO2:%d}"%(wlight,wTemp,wMHMH,wHumid,wCO2))
    txt_wsent.configure(text='发送成功')

def man_water():
    client.publish("SW_LED", "water_man")

def man_wind():
    client.publish("SW_LED", "wind_man")
	
def man_led_up():
    client.publish("SW_LED", "light_man_up")
	
def man_led_down():
    client.publish("SW_LED", "light_man_down")

#========================MQTT初始化========================================================
client = mqtt.Client()
client.username_pw_set(MQTTNAME, MQTTPASW)  # 必须设置，否则会返回「Connected with result code 4」
client.on_connect = on_connect
client.on_message = on_message
client.connect(MQTTHOST, MQTTPORT, 60)
    #client.loop_forever()
user = 'uudp5h2/iot_light_mqttfx_baidu_jx'
client.user_data_set(user)
client.loop_start()
#==========================================================================================


root = Tk()
root.title('大棚')
root.configure(background='white')
root.geometry('1280x720')  #本电脑的分辨率是1920x1080
f = 'D:\\dapeng\\dapeng.csv'
file_save = open(f,'w',encoding='utf-8',newline='')
csv_writer = csv.writer(file_save) 
csv_writer.writerow(["时间","温度/℃","湿度/%RH","CO2/ppm","土壤湿度","光照强度"])
cv = Canvas(root,bg='white')
cv.create_line(640,0,640,720)
cv.create_line(0,680,1280,680)
cv.create_line(640,230,1280,230)
cv.create_line(640,460,1280,460)
cv.place(relx = 0,rely = 0, relwidth = 1, relheight=1)

var = IntVar()
var.set(1)

rd_auto = Radiobutton(root,text='自动',variable=var,value=1,command=Mysel,bg='white',activebackground='white')
rd_auto.place(relx = 0.501,rely = 0.001, relwidth = 0.048, relheight=0.085)
rd_man = Radiobutton(root,text='手动',variable=var,value=0,command=Mysel,bg='white',activebackground='white')
rd_man.place(relx = 0.501,rely = 0.64, relwidth = 0.048, relheight=0.085)

warningVar1 = IntVar()
btn_warning = Checkbutton(root,text='报警',variable = warningVar1,bg='white',onvalue=1,offvalue=0,command=warning_choose)
btn_warning.place(relx = 0.501,rely = 0.321, relwidth = 0.048, relheight=0.055)

#========================图片读取==========================================================
Img_exit = Image.open('D:\\dapeng\\图标\\go to app-v5VMCMhT.png').resize((64,64))
Img_exit = ImageTk.PhotoImage(Img_exit)
Img_bazzer_on = Image.open('D:\\dapeng\\图标\\sound-xxb1IGTe.png').resize((64,64))
Img_bazzer_on = ImageTk.PhotoImage(Img_bazzer_on)
Img_bazzer_off = Image.open('D:\\dapeng\\图标\\notification-3C4939OX.png').resize((64,64))
Img_bazzer_off = ImageTk.PhotoImage(Img_bazzer_off)
Img_Temp = Image.open('D:\\dapeng\\图标\\temperature-POHJ6JkM.png').resize((123,123))
Img_Temp = ImageTk.PhotoImage(Img_Temp)
Img_Humid = Image.open('D:\\dapeng\\图标\\humidity-LPSX0B8g.png').resize((123,123))
Img_Humid = ImageTk.PhotoImage(Img_Humid)
Img_CO2 = Image.open('D:\\dapeng\\图标\\breeze-LJQ2EJwx.png').resize((123,123))
Img_CO2 = ImageTk.PhotoImage(Img_CO2)
Img_MHMH = Image.open('D:\\dapeng\\图标\\love anniversary-VYDz4Kp9.png').resize((123,123))
Img_MHMH = ImageTk.PhotoImage(Img_MHMH)
Img_light = Image.open('D:\\dapeng\\图标\\idea-TljNlG_X.png').resize((123,123))
Img_light = ImageTk.PhotoImage(Img_light)
Img_visit = Image.open('D:\\dapeng\\图标\\可视化.png').resize((280,280))
Img_visit = ImageTk.PhotoImage(Img_visit)
Img_meter = Image.open('D:\\dapeng\\图标\\仪表盘.png').resize((280,280))
Img_meter = ImageTk.PhotoImage(Img_meter)
Img_sent = Image.open('D:\\dapeng\\图标\\paper--bAmT2eu.png').resize((123,123))
Img_sent = ImageTk.PhotoImage(Img_sent)
Img_water = Image.open('D:\\dapeng\\图标\\rainy weather-b8IrO1u1.png').resize((80,80))
Img_water = ImageTk.PhotoImage(Img_water)
Img_wind = Image.open('D:\\dapeng\\图标\\energy-yav6U_ch.png').resize((80,80))
Img_wind = ImageTk.PhotoImage(Img_wind)
Img_led_up = Image.open('D:\\dapeng\\图标\\Bright-ArGnwdHt.png').resize((80,80))
Img_led_up = ImageTk.PhotoImage(Img_led_up)
Img_led_down = Image.open('D:\\dapeng\\图标\\Bright-ORfSnv8q.png').resize((80,80))
Img_led_down = ImageTk.PhotoImage(Img_led_down)
#================================================================================================


#=====================背景图=============================================================
lb_Temp = Label(root,image=Img_Temp,bg='white')
lb_Temp.place(relx = 0.308,rely = 0.05, relwidth = 0.096, relheight=0.17)
lb_Humid = Label(root,image=Img_Humid,bg='white')
lb_Humid.place(relx = 0.308,rely = 0.22, relwidth = 0.096, relheight=0.17)
lb_CO2 = Label(root,image=Img_CO2,bg='white')
lb_CO2.place(relx = 0.01,rely = 0.1, relwidth = 0.096, relheight=0.17)
lb_MHMH = Label(root,image=Img_MHMH,bg='white')
lb_MHMH.place(relx = 0.01,rely = 0.25, relwidth = 0.096, relheight=0.17)
lb_light = Label(root,image=Img_light,bg='white')
lb_light.place(relx = 0.01,rely = 0.4, relwidth = 0.096, relheight=0.17)
lb_visit = Label(root,image=Img_visit,bg='white')
lb_visit.place(relx = 0.025,rely = 0.555, relwidth = 0.21875, relheight=0.3889)
lb_meter = Label(root,image=Img_meter,bg='white')
lb_meter.place(relx = 0.259,rely = 0.555, relwidth = 0.21875, relheight=0.3889)


#====================文本框==============================================================
txt_Temp = Label(root,text='25℃',bg='white')
txt_Temp.place(relx = 0.368,rely = 0.05, relwidth = 0.096, relheight=0.17)

txt_Humid = Label(root,text='40%RH',bg='white')
txt_Humid.place(relx = 0.372,rely = 0.22, relwidth = 0.096, relheight=0.17)

txt_CO2 = Label(root,text='400ppm',bg='white')
txt_CO2.place(relx = 0.09,rely = 0.1, relwidth = 0.096, relheight=0.17)

txt_MHMH = Label(root,text='78',bg='white')
txt_MHMH.place(relx = 0.079,rely = 0.25, relwidth = 0.096, relheight=0.17)

txt_light = Label(root,text='200',bg='white')
txt_light.place(relx = 0.079,rely = 0.4, relwidth = 0.096, relheight=0.17)

txt_file = Label(root,text='数据储存在:'+f,bg='white')
txt_file.place(relx = 0.24,rely = 0.44, relwidth = 0.2, relheight=0.1)

txt_visit = Label(root,text='可视化:',bg='white')
txt_visit.place(relx = 0,rely = 0.56, relwidth = 0.05, relheight=0.1)

txt_meter = Label(root,text='仪表盘:',bg='white')
txt_meter.place(relx = 0.23,rely = 0.56, relwidth = 0.05, relheight=0.1)

txt_see = Label(root,text='观测',bg='white')
txt_see.place(relx = 0.18,rely = 0.95, relwidth = 0.1, relheight=0.05)

txt_control = Label(root,text='控制',bg='white')
txt_control.place(relx = 0.71,rely = 0.95, relwidth = 0.1, relheight=0.05)

txt_clight = Label(root,text='光照强度:',bg='white',font=('华文新魏',15))
txt_clight.place(relx = 0.51,rely = 0.056, relwidth = 0.076, relheight=0.06)

txt_cTemp = Label(root,text='温度:',bg='white',font=('华文新魏',15))
txt_cTemp.place(relx = 0.51,rely = 0.116, relwidth = 0.076, relheight=0.06)

txt_cMHMH = Label(root,text='土壤湿度:',bg='white',font=('华文新魏',15))
txt_cMHMH.place(relx = 0.51,rely = 0.176, relwidth = 0.076, relheight=0.06)

txt_csent = Label(root,text='请输入数字',bg='white',font=('华文新魏',15))
txt_csent.place(relx = 0.701,rely = 0.001, relwidth = 0.1, relheight=0.085)

txt_wsent = Label(root,text='请输入数字',bg='white',font=('华文新魏',15))
txt_wsent.place(relx = 0.701,rely = 0.321, relwidth = 0.1, relheight=0.085)

txt_warning = Label(root,text='报警阈值',bg='white')
#txt_warning.place(relx = 0.501,rely = 0.321, relwidth = 0.068, relheight=0.048)

txt_wlight = Label(root,text='光照强度:',bg='white',font=('华文新魏',15))
txt_wlight.place(relx = 0.51,rely = 0.38, relwidth = 0.076, relheight=0.06)

txt_wTemp = Label(root,text='温度:',bg='white',font=('华文新魏',15))
txt_wTemp.place(relx = 0.51,rely = 0.435, relwidth = 0.076, relheight=0.06)

txt_wMHMH = Label(root,text='土壤湿度:',bg='white',font=('华文新魏',15))
txt_wMHMH.place(relx = 0.51,rely = 0.51, relwidth = 0.076, relheight=0.06)

txt_wHumid = Label(root,text='湿度:',bg='white',font=('华文新魏',15))
txt_wHumid.place(relx = 0.68,rely = 0.405, relwidth = 0.076, relheight=0.06)

txt_wCO2 = Label(root,text='CO2:',bg='white',font=('华文新魏',15))
txt_wCO2.place(relx = 0.68,rely = 0.48, relwidth = 0.076, relheight=0.06)

#====================输入文本框==========================================================
inp_clight = Entry(root,bd=3)
inp_clight.place(relx = 0.586,rely = 0.056, relwidth = 0.076, relheight=0.06)
inp_cTemp = Entry(root,bd=3)
inp_cTemp.place(relx = 0.586,rely = 0.116, relwidth = 0.076, relheight=0.06)
inp_cMHMH = Entry(root,bd=3)
inp_cMHMH.place(relx = 0.586,rely = 0.176, relwidth = 0.076, relheight=0.06)

inp_wlight = Entry(root,bd=3)
inp_wlight.place(relx = 0.586,rely = 0.38, relwidth = 0.076, relheight=0.06)
inp_wTemp = Entry(root,bd=3)
inp_wTemp.place(relx = 0.586,rely = 0.445, relwidth = 0.076, relheight=0.06)
inp_wMHMH = Entry(root,bd=3)
inp_wMHMH.place(relx = 0.586,rely = 0.51, relwidth = 0.076, relheight=0.06)
inp_wHumid = Entry(root,bd=3)
inp_wHumid.place(relx = 0.756,rely = 0.405, relwidth = 0.076, relheight=0.06)
inp_wCO2 = Entry(root,bd=3)
inp_wCO2.place(relx = 0.756,rely = 0.48, relwidth = 0.076, relheight=0.06)


#====================前景图==============================================================
lb_bazzer = Label(root,image=Img_bazzer_off,relief=RAISED,bg='white')
lb_bazzer.place(relx = 0.05,rely = 0, relwidth = 0.05, relheight=0.0889)


#====================按钮=============================================================
btn_exit = Button(root,text='exit',image=Img_exit,command=root.destroy)
btn_exit.place(relx = 0,rely = 0, relwidth = 0.05, relheight=0.0889)
btn_csent = Button(root,text='csent',image=Img_sent,command=push_control,bg='white')
btn_csent.place(relx = 0.95,rely = 0, relwidth = 0.05, relheight=0.0889)

btn_mwater = Button(root,image=Img_water,command=man_water,bg='white')
btn_mwater.place(relx = 0.51,rely = 0.725, relwidth = 0.0625, relheight=0.1111)
btn_mwind = Button(root,image=Img_wind,command=man_wind,bg='white')
btn_mwind.place(relx = 0.71,rely = 0.725, relwidth = 0.0625, relheight=0.1111)
btn_cmled_up = Button(root,image=Img_led_up,command=man_led_up,bg='white')
btn_cmled_up.place(relx = 0.91,rely = 0.65, relwidth = 0.0625, relheight=0.1111)
btn_cmled_down = Button(root,image=Img_led_down,command=man_led_down,bg='white')
btn_cmled_down.place(relx = 0.91,rely = 0.82, relwidth = 0.0625, relheight=0.1111)

btn_wsent = Button(root,image=Img_sent,command=push_warning,bg='white')
btn_wsent.place(relx = 0.95,rely = 0.321, relwidth = 0.05, relheight=0.0889)



Mysel()           #调用一次单选框
root.mainloop()
file_save.close()