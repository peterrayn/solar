import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.linear_model import LinearRegression
from sklearn.ensemble import RandomForestRegressor
from sklearn.metrics import mean_squared_error, r2_score
from sklearn.preprocessing import StandardScaler
import xgboost as xgb

# ==========================================
# 1. 生成模拟数据 (假设每10秒采集一次)
# ==========================================
np.random.seed(42)
n_samples = 3000

# 生成时间索引
time_index = pd.date_range(start='2023-01-01 08:00:00', periods=n_samples, freq='10S')

# 模拟照度 (随时间变化的曲线 + 噪音)
illuminance = np.sin(np.linspace(0, 3.14, n_samples)) * 1000 + np.random.normal(0, 50, n_samples)
illuminance = np.clip(illuminance, 0, None) # 照度不能为负

# 模拟温度 (与照度有一定相关性 + 延迟 + 噪音)
temperature = 15 + illuminance * 0.01 + np.random.normal(0, 1, n_samples)

# 模拟当前发电量 (照度 * 温度系数 * 转换效率 + 噪音)
# 温度过高会降低光伏效率 (简单模拟负相关)
efficiency = 0.20 * (1 - 0.004 * (temperature - 25)) 
power = illuminance * efficiency + np.random.normal(0, 5, n_samples)
power = np.clip(power, 0, None)

df = pd.DataFrame({
    'time': time_index,
    'temperature': temperature,
    'illuminance': illuminance,
    'power': power
})

# ==========================================
# 2. 特征工程 (构造过去4个时间点的数据 + 预测未来)
# ==========================================
# 提取时间特征 (将时间转化为模型可以理解的数字)
df['hour'] = df['time'].dt.hour
df['minute'] = df['time'].dt.minute
df['second'] = df['time'].dt.second

# 构造过去4个时间点的特征 (t, t-1, t-2, t-3)
# 假设当前是 t 时刻
for lag in range(0, 4):
    df[f'temp_lag_{lag}'] = df['temperature'].shift(lag)
    df[f'ill_lag_{lag}'] = df['illuminance'].shift(lag)

# 定义目标变量：预测未来10秒的发电量 (因为数据是10秒一采，即预测下一行的power)
df['target_power_plus_10s'] = df['power'].shift(-1)

# 删除因为shift产生的NaN行
df = df.dropna()

# 定义特征列 (X) 和 目标列 (y)
feature_cols = ['hour', 'minute', 'second'] + \
               [f'temp_lag_{lag}' for lag in range(4)] + \
               [f'ill_lag_{lag}' for lag in range(4)]

X = df[feature_cols]
y = df['target_power_plus_10s']

# ==========================================
# 3. 数据集划分与标准化
# ==========================================
# 注意：时间序列预测绝不能打乱顺序 (shuffle=False)
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, shuffle=False)

# 标准化 (对线性回归很重要，对树模型影响不大)
scaler = StandardScaler()
X_train_scaled = scaler.fit_transform(X_train)
X_test_scaled = scaler.transform(X_test)

# ==========================================
# 4. 模型训练与评估
# ==========================================
models = {
    "Linear Regression": LinearRegression(),
    "Random Forest": RandomForestRegressor(n_estimators=100, max_depth=10, random_state=42),
    "XGBoost": xgb.XGBRegressor(n_estimators=100, learning_rate=0.1, max_depth=5, random_state=42)
}

print("=== 模型预测结果评估 (预测未来10秒发电量) ===")
for name, model in models.items():
    # 训练模型
    if name == "Linear Regression":
        model.fit(X_train_scaled, y_train)
        y_pred = model.predict(X_test_scaled)
    else:
        model.fit(X_train, y_train)
        y_pred = model.predict(X_test)
    
    # 计算评估指标
    rmse = np.sqrt(mean_squared_error(y_test, y_pred))
    r2 = r2_score(y_test, y_pred)
    
    print(f"\n{name}:")
    print(f"  RMSE (均方根误差): {rmse:.4f}")
    print(f"  R2 Score (决定系数): {r2:.4f}")

# 如果想看 XGBoost 的特征重要性
import matplotlib.pyplot as plt
xgb_model = models["XGBoost"]
xgb.plot_importance(xgb_model, max_num_features=10)
plt.title("XGBoost Feature Importance")
plt.show()