import pandas as pd
from server import app, db, SensorRecord # 请替换为你实际的导入路径
import matplotlib.pyplot as plt
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.linear_model import LinearRegression
from sklearn.ensemble import RandomForestRegressor
from sklearn.metrics import mean_squared_error, mean_absolute_error, r2_score
from sklearn.preprocessing import StandardScaler
import xgboost as xgb
import random
def fetch_data_from_db():
    # 需要在 Flask 上下文中运行数据库查询
    with app.app_context():
        # 按照时间升序查询所有数据（时间序列预测必须按时间排序）
        # 提示：如果数据量太大，可以加 .limit(10000) 取最近的数据
        records = SensorRecord.query.order_by(SensorRecord.timestamp.asc()).all()
        
        if not records:
            print("数据库中没有数据！")
            return None
            
        # 使用你模型里自带的 to_dict 方法，将对象列表转换为字典列表
        data_list = [record.to_dict() for record in records]
        
        # 转换为 Pandas DataFrame，这是机器学习最常用的数据结构
        df = pd.DataFrame(data_list)
        return df
np.random.seed(42)
# 获取数据
df = fetch_data_from_db()
df = df[df["power"] <= 250]
df = df[df["t1"] <= 50]
df = df.sample(frac=1).reset_index(drop=True)
print(len(df))

coeff_v = -0.004  # 每°C变化

delta_T = 15+np.random.randn(len(df))
# delta_T = np.random.randint(-10, 20, size=len(df))
df[["t1","t2","t3","t4"]] = df[["t1","t2","t3","t4"]].add(delta_T, axis=0)
df["voltage"] = df["voltage"] * (1 + coeff_v * delta_T)


df['T']=(df["t1"]+df["t2"]+df["t3"]+df["t4"])/4

df['current']=df['power']/df['voltage']

plt.figure(figsize=(18,9))

plt.scatter(df['id'], df['T'], label='avg temperature')
plt.scatter(df['id'], df['current'], label='current')
plt.scatter(df['id'], df['voltage'], label='voltage')
plt.scatter(df['id'], df['power'], label='power')

plt.xlabel('Sample ID')  # 横坐标
plt.ylabel('Value (Temperature / Current / Voltage / Power)')  # 纵坐标

plt.title('Sensor Measurements Overview')  # 标题

plt.legend(loc='lower left')
plt.show()
# while(1):
#     pass

models = {
    "Linear Regression": LinearRegression(),
    "Random Forest": RandomForestRegressor(n_estimators=100, max_depth=10, n_jobs=-1, random_state=42),
    "XGBoost": xgb.XGBRegressor(n_estimators=100, learning_rate=0.1, max_depth=6, random_state=42)
}
import pandas as pd
import numpy as np
import xgboost as xgb

import pandas as pd
import numpy as np
import xgboost as xgb
from sklearn.model_selection import train_test_split

def compare_strategies_for_thesis(df):
    print("========== 毕业论文：基于测试集 (未知环境) 的策略仿真对比 ==========\n")
    
    # 1. 数据清洗与重建
    df = df.dropna(subset=['t1', 'voltage']).copy()
    
    
    df['avg_T'] = df[['t1', 't2', 't3', 't4']].mean(axis=1)
    
    # 重构功率数据 P = U^2 / R (假设负载 R=10欧姆)
    LOAD_RESISTANCE = 10.0
    df['synthetic_power'] = (df['voltage'] ** 2) / LOAD_RESISTANCE
    

    # --- 系统物理参数设定 ---
    PUMP_POWER = 0.36
    SPRAY_DURATION = 2
    PUMP_ENERGY = PUMP_POWER * SPRAY_DURATION
    TEMP_DROP = 3

    # ==============================================================
    # 2. 严格的数据集划分 (70% 训练模型，30% 作为纯净的测试集进行仿真)
    # ==============================================================
    # 对于物联网时序数据，最好不要随机打乱，而是按时间截断
    split_idx = int(len(df) * 0.7)
    df_train = df.iloc[:split_idx].copy()
    df_test = df.iloc[split_idx:].copy()  # <<< 我们只在 df_test 上做仿真！
    
    print(f">> 数据集划分完成：训练集 {len(df_train)} 条，测试集(未知环境) {len(df_test)} 条。")

    # ==============================================================
    # 3. 训练模型 (只允许看 df_train)
    # ==============================================================
    features = ['t1', 't2', 't3', 't4', 'illuminance']
    X_train = df_train[features]
    y_train = df_train['synthetic_power']
    
    model =models["XGBoost"]
    model.fit(X_train, y_train)
    print(">> AI 发电量预测模型训练完成，准备在【未知测试集】进行仿真...\n")

    # ==============================================================
    # 策略 A：传统温度阈值控制 (在测试集 df_test 上运行)
    # ==============================================================
    # 阈值可以根据历史经验（训练集）设定，而不是作弊看测试集
    dynamic_threshold = 48
    
    trigger_A = (df_test['avg_T'] > dynamic_threshold) & (df_test['illuminance'] > 100)
    
    original_power_A = df_test.loc[trigger_A, 'synthetic_power']
    real_gain_power_A = original_power_A * TEMP_DROP * 0.004 
    
    total_pump_energy_A = trigger_A.sum() * PUMP_ENERGY
    total_gain_energy_A = real_gain_power_A.sum() * SPRAY_DURATION
    net_profit_A = total_gain_energy_A - total_pump_energy_A

    # ==============================================================
    # 策略 B：ML 智能预测寻优控制 (在测试集 df_test 上运行)
    # ==============================================================
    # 步骤1：AI 预测测试集当前环境下的功率
    pred_power_current = model.predict(df_test[features])
    
    # 步骤2：构造测试集的虚拟降温环境
    df_test_simulated = df_test[features].copy()
    df_test_simulated['t1'] -= TEMP_DROP
    df_test_simulated['t2'] -= TEMP_DROP
    df_test_simulated['t3'] -= TEMP_DROP
    df_test_simulated['t4'] -= TEMP_DROP
    
    # 步骤3：AI 预测降温后的功率
    pred_power_spray = model.predict(df_test_simulated)
    
    # 步骤4：计算预测增益 (纯AI判断)
    pred_gain_power = pred_power_spray - pred_power_current
    pred_gain_energy = pred_gain_power * SPRAY_DURATION
    
    # 步骤5：智能决策 -> 如果预测收益大于能耗，则执行
    trigger_B = pred_gain_energy > PUMP_ENERGY
    
    # 真实收益结算 (客观裁判结算 df_test 的客观物理收益)
    original_power_B = df_test.loc[trigger_B, 'synthetic_power']
    real_gain_power_B = original_power_B * TEMP_DROP * 0.004
    
    total_pump_energy_B = trigger_B.sum() * PUMP_ENERGY
    total_gain_energy_B = real_gain_power_B.sum() * SPRAY_DURATION
    net_profit_B = total_gain_energy_B - total_pump_energy_B

    # ==============================================================
    # 输出论文所需对比结果
    # ==============================================================
    results = pd.DataFrame({
        "评估指标": ["判定条件", "触发次数(测试集)", "总发电增量 (Ws)", "水泵耗电 (Ws)", "系统实际净收益 (Ws)"],
        f"传统温度阈值法": [f"T > {dynamic_threshold:.1f}℃", trigger_A.sum(), round(total_gain_energy_A, 2), round(total_pump_energy_A, 2), round(net_profit_A, 2)],
        "基于ML的预测控制": ["AI预测增益 > 水泵耗能", trigger_B.sum(), round(total_gain_energy_B, 2), round(total_pump_energy_B, 2), round(net_profit_B, 2)]
    })
    
    print(results.to_string(index=False))
    print("-" * 75)

compare_strategies_for_thesis(df)