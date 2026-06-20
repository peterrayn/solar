import pandas as pd
from server import app, db, SensorRecord # 请替换为你实际的导入路径
import matplotlib.pyplot as plt
import time
import numpy as np
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

# 获取数据
df = fetch_data_from_db()
df = df[df["power"] <= 250]
df = df[df["t1"] <= 50]
df = df.sample(frac=1).reset_index(drop=True)
print(len(df))

coeff_v = -0.004  # 每°C变化
np.random.seed(42)
delta_T = 15+np.random.randn(len(df))
# delta_T = np.random.randint(-10, 20, size=len(df))
df[["t1","t2","t3","t4"]] = df[["t1","t2","t3","t4"]].add(delta_T, axis=0)
df["voltage"] = df["voltage"] * (1 + coeff_v * delta_T)


df['T']=(df["t1"]+df["t2"]+df["t3"]+df["t4"])/4

df['current']=df['power']/df['voltage']




fig, ax1 = plt.subplots(figsize=(18, 9))

ax1.plot(df['id'],df['t1'],label='t1')
ax1.plot(df['id'],df['t2'],label='t2')
ax1.plot(df['id'],df['t3'],label='t3')
ax1.plot(df['id'],df['t4'],label='t4')
ax1.plot(df['id'],df['stm32_temp'],label='stm32 temperature', linestyle='--')
ax1.plot(df['id'],df['current'],label='current')
ax1.plot(df['id'],df['voltage'],label='voltage')
ax1.plot(df['id'],df['power'],label='power')

ax1.set_ylabel('Electrical Variables')






# 右轴（关键）
ax2 = ax1.twinx()
ax2.plot(df['id'], df['illuminance'], color='orange', label='illuminance')
ax2.set_ylabel('Illuminance')
lines1, labels1 = ax1.get_legend_handles_labels()
lines2, labels2 = ax2.get_legend_handles_labels()
ax1.legend(lines1 + lines2, labels1 + labels2, loc='lower left')
plt.show()
# time.sleep(100000)

import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.linear_model import LinearRegression
from sklearn.ensemble import RandomForestRegressor
from sklearn.metrics import mean_squared_error, mean_absolute_error, r2_score
from sklearn.preprocessing import StandardScaler
import xgboost as xgb

def train_and_evaluate_models(df):
    if df is None or df.empty:
        return
        
    print(f"成功加载数据，总行数: {len(df)}")

    # ==========================================
    # 1. 数据预处理
    # ==========================================
    # 将字符串格式的时间戳转换为 datetime 对象
    df['timestamp'] = pd.to_datetime(df['timestamp'], format='%Y-%m-%d %H:%M:%S')
    
    # 提取时间特征：模型无法直接理解 datetime，必须拆解成数字
    # 根据你的采样频率（比如几秒或几分钟），可以提取小时、分钟、甚至秒
    df['hour'] = df['timestamp'].dt.hour
    df['minute'] = df['timestamp'].dt.minute
    # df['second'] = df['timestamp'].dt.second # 如果你的数据是秒级采样的，把这个注释放开
    
    # ==========================================
    # 2. 构造预测目标 (Label)
    # ==========================================
    # 我们要预测“下一个时间点”的 voltage
    # shift(-1) 会把下一行的 voltage 移到当前行
    # df['target_next_power'] = df['power'].shift(-1)
    
    # 因为最后一行没有“下一行”，它的 target_next_voltage 会变成 NaN，必须删掉
    # df = df.dropna(subset=['target_next_power', 't1', 't2', 't3', 't4', 'illuminance'])

    # ==========================================
    # 3. 定义特征 (X) 和 目标 (y)
    # ==========================================
    # 根据你的要求，取 时间、照度 和 t1-t4
    # features = ['hour', 'minute', 'illuminance', 't1', 't2', 't3', 't4', 'power']
    features = [ 'illuminance', 't1', 't2', 't3', 't4']
    X = df[features]
    y = df['voltage']

    # ==========================================
    # 4. 划分训练集和测试集
    # ==========================================
    # 重要提示：时间序列千万不能打乱！shuffle=False
    # 用前 80% 的时间数据训练，后 20% 的时间数据测试
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, shuffle=False)

    # 数据标准化 (针对线性回归和神经网络很有用，树模型其实不需要，但统一下比较好)
    scaler = StandardScaler()
    X_train_scaled = scaler.fit_transform(X_train)
    X_test_scaled = scaler.transform(X_test)

    # ==========================================
    # 5. 训练三种模型
    # ==========================================
    models = {
        "Linear Regression (线性回归)": LinearRegression(),
        "Random Forest (随机森林)": RandomForestRegressor(n_estimators=100, max_depth=10, n_jobs=-1, random_state=42),
        "XGBoost": xgb.XGBRegressor(n_estimators=100, learning_rate=0.1, max_depth=6, random_state=42)
    }

    print("\n" + "="*40)
    print("预测目标: 下一个时间点的 Voltage (电压)")
    print("使用特征:", features)
    print("="*40)

    for name, model in models.items():
        # 训练
        if "Linear Regression" in name:
            model.fit(X_train_scaled, y_train)
            y_pred = model.predict(X_test_scaled)
        else:
            model.fit(X_train, y_train) # 树模型直接用未标准化的数据即可
            y_pred = model.predict(X_test)
        
        # 评估指标
        mae = mean_absolute_error(y_test, y_pred) # 平均绝对误差 (直观反映预测电压偏离了多少 V)
        rmse = np.sqrt(mean_squared_error(y_test, y_pred)) # 均方根误差 (对大的预测偏差更敏感)
        r2 = r2_score(y_test, y_pred) # R2 分数 (越接近1越好)
        
        print(f"\n【{name}】 评估结果:")
        print(f"  MAE (平均绝对误差): {mae:.4f} V")
        print(f"  RMSE (均方根误差): {rmse:.4f} V")
        print(f"  R2 Score (拟合度): {r2:.4f}")

    # ==========================================
    # 6. (可选) 查看 XGBoost 认为哪个特征最重要
    # ==========================================
    # 通过打印特征重要性，你可以知道是照度影响大，还是 t1 影响大
    xgb_model = models["XGBoost"]
    feature_importances = pd.DataFrame({
        'Feature': features,
        'Importance': xgb_model.feature_importances_
    }).sort_values(by='Importance', ascending=False)
    
    print("\n--- XGBoost 特征重要性排名 ---")
    print(feature_importances)


# 执行训练逻辑 (假设你已经通过上面的函数拿到了 df)
if __name__ == '__main__':
    # 为了防止你还没连数据库跑不起来，如果 df 是空的话，可以用假数据测一下这套逻辑
    # df = pd.DataFrame(...) 
    train_and_evaluate_models(df)