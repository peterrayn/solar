# from flask import Flask,redirect,url_for,render_template,request,session,jsonify
# from flask_sqlalchemy import SQLAlchemy
# from datetime import datetime

# app= Flask(__name__)

# app.config['SQLALCHEMY_DATABASE_URI']='sqlite:///users.sqlite3'
# app.config["SQLALCHEMY_TRACK_MODIFICATIONS"]=False
# db=SQLAlchemy(app)






# class user_info(db.Model):
#     _id =db.Column("id",db.Integer,primary_key=True)
#     num=db.Column(db.Integer)

#     def __init__(self,num):
#         self.num=num

# # @app.route("/check")
# # def check():
# #     users = user_info.query.all()
# #     return jsonify([{"id": u._id,"num":u.num} for u in users])



# @app.route("/")
# def home():
#     return "helloworld"
#     # return render_template("index.html")

# @app.route("/dataupload", methods=["GET"])
# def dataupload():

#     temperature = request.args.get("tem")
#     humidity=request.args.get("hum")
#     illuminance=request.args.get("ill")
    
#     # "hearme?1&52&12&34&101&23&34&23?"
#     return f"hearme?12?"

# if __name__=="__main__":
#     with app.app_context():
#         db.create_all()
#     app.run("0.0.0.0",port=80)




from flask import Flask,redirect,url_for,render_template,request,session,jsonify
from flask import Flask, jsonify, request
from flask_cors import CORS
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime
import threading
import random
import time

app = Flask(__name__)
CORS(app)

# 配置 SQLAlchemy (使用本地 SQLite 数据库)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///sensor_telemetry.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
db = SQLAlchemy(app)

# ==== 1. 定义数据模型 (ORM) ====
class SensorRecord(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    timestamp = db.Column(db.String(30))
    t1 = db.Column(db.Float)
    t2 = db.Column(db.Float)
    t3 = db.Column(db.Float)
    t4 = db.Column(db.Float)
    stm32_temp = db.Column(db.Float) # 特殊参数：主控芯片温度
    illuminance = db.Column(db.Float)
    current = db.Column(db.Float)
    voltage = db.Column(db.Float)
    power = db.Column(db.Float)
    is_spraying = db.Column(db.Boolean)
    mode = db.Column(db.String(10))
    def __repr__(self):
        return (
            f"<SensorRecord id={self.id}, timestamp={self.timestamp}, "
            f"t1={self.t1}, t2={self.t2}, t3={self.t3}, t4={self.t4}, "
            f"stm32_temp={self.stm32_temp}, illuminance={self.illuminance}, "
            f"current={self.current}, voltage={self.voltage}, power={self.power}, "
            f"is_spraying={self.is_spraying}, mode={self.mode}>"
        )
    def to_dict(self):
        return {c.name: getattr(self, c.name) for c in self.__table__.columns}

# ==== 2. 系统控制全局变量 ====
sys_state = {
    "mode": "auto",
    "manual_spray": False,
    "is_spraying": False,
    "base_temp":40
}

# ==== 3. 硬件模拟守护线程 (Background Thread) ====
def hardware_simulator():
    """ 模拟硬件不间断地产生数据并存入数据库 """
    with app.app_context():
        db.create_all() # 确保数据库表已创建
        
        while True:
            global sys_state
            
            # 环境与电学计算
            illuminance = random.uniform(85000, 95000)
            current = illuminance / 10000 + random.uniform(-0.1, 0.1)
            voltage = 36.5 + random.uniform(-0.5, 0.5)
            power = voltage * current
            
            # 控制逻辑计算
            if sys_state["mode"] == "auto":
                if sys_state["base_temp"] > 42.0:
                    sys_state["is_spraying"] = True
                elif sys_state["base_temp"] < 35.0:
                    sys_state["is_spraying"] = False
            else:
                sys_state["is_spraying"] = sys_state["manual_spray"]

            # 热力学模拟 (环境热容升温 vs 喷水骤降)
            if sys_state["is_spraying"]:
                sys_state["base_temp"] = max(sys_state["base_temp"] - random.uniform(0.5, 1.2), 25.0)
            else:
                sys_state["base_temp"] += (illuminance / 100000) * random.uniform(0.2, 0.6)

            bt = sys_state["base_temp"]
            # 芯片温度通常比环境/表面温度高，且发热具有滞后性
            stm32_t = bt + random.uniform(8.0, 12.0)

            # 创建数据库记录
            new_record = SensorRecord(
                timestamp=datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
                t1=round(bt + random.uniform(0, 0.5), 2),
                t2=round(bt + random.uniform(0.5, 1.0), 2),
                t3=round(bt + random.uniform(-0.5, 0), 2),
                t4=round(bt + random.uniform(-0.2, 0.3), 2),
                stm32_temp=round(stm32_t, 2),
                illuminance=round(illuminance, 0),
                current=round(current, 3),
                voltage=round(voltage, 2),
                power=round(power, 2),
                is_spraying=sys_state["is_spraying"],
                mode=sys_state["mode"]
            )
            db.session.add(new_record)
            db.session.commit()
            
            time.sleep(1.5) # 每 1.5 秒采样一次


@app.route("/solarsystem")
def home():
    # return "helloworld"
    return render_template("home.html")


# From MCU
@app.route("/dataupload", methods=["GET"])
def dataupload():
    if request.args.get("pass")!="123456":
        return "",403
    t1 = float(request.args.get("t1"))
    t2 = float(request.args.get("t2"))
    t3 = float(request.args.get("t3"))
    t4 = float(request.args.get("t4"))
    stm = float(request.args.get("stm"))
    illum = float(request.args.get("illum"))
    current = float(request.args.get("current"))
    power = float(request.args.get("power"))
    voltage = float(request.args.get("voltage"))

    avarage_temp=(t1+t2+t3+t4)/4.0

    if sys_state["mode"] == "auto":
        if avarage_temp > 42.0:
            sys_state["is_spraying"] = True
        elif avarage_temp < 36.0:
            sys_state["is_spraying"] = False
    else:
        sys_state["is_spraying"] = sys_state["manual_spray"]



    now_time=datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    new_record = SensorRecord(
        timestamp=now_time,
        t1=t1,
        t2=t2,
        t3=t3,
        t4=t4,
        stm32_temp=stm,
        illuminance=illum,
        current=current,
        voltage=voltage,
        power=power,
        is_spraying=sys_state["is_spraying"],
        mode=sys_state["mode"]
    )
    db.session.add(new_record)
    db.session.commit()



    # timestamp and ismotor
    # "hearme?1&52&12&34&101&23&34&23?"
    if sys_state["is_spraying"]:
        is_spray=1
    else:
        is_spray=0


    return f"hearme?{datetime.now().strftime('%Y%m%d')}&{datetime.now().strftime('%H%M%S')}&{is_spray}??"



# 把数据给前端
@app.route('/api/data', methods=['GET'])
def get_sensor_data():
    """ 
    增量数据同步接口：
    如果 last_id = 0, 返回最新的 50 条记录用于初始化页面。
    如果 last_id > 0, 仅返回大于该 ID 的新增记录，避免重复请求。
    """
    last_id = request.args.get('last_id', default=0, type=int)
    
    if last_id == 0:
        # 初次加载：获取最新50条（需先倒序取，再正序返回）
        records = SensorRecord.query.order_by(SensorRecord.id.desc()).limit(50).all()
        records.reverse()
    else:
        # 轮询加载：获取增量
        records = SensorRecord.query.filter(SensorRecord.id > last_id).order_by(SensorRecord.id.asc()).all()
        
    return jsonify([r.to_dict() for r in records])



@app.route('/api/control', methods=['POST'])
def control_actuator():
    global sys_state
    req_data = request.json
    if "mode" in req_data:
        sys_state["mode"] = req_data["mode"]
        sys_state["manual_spray"] = False 
    if "spray" in req_data and sys_state["mode"] == "manual":
        sys_state["manual_spray"] = bool(req_data["spray"])
    return jsonify({"status": "success"})

import werkzeug.serving

# 强制 Werkzeug 认为所有请求都应该是长连接
class ForeverAliveHandler(werkzeug.serving.WSGIRequestHandler):
    def should_keep_alive(self):
        return True
if __name__ == '__main__':
    # 启动后台硬件模拟器
    # sim_thread = threading.Thread(target=hardware_simulator, daemon=True)
    # sim_thread.start()
    with app.app_context():
        db.create_all() # 确保数据库表已创建
    app.run(host='0.0.0.0', port=2003,request_handler=ForeverAliveHandler) 