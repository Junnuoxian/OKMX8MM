CREATE TABLE IF NOT EXISTS oil_sensor_metrics (
    id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
    device_name VARCHAR(127) COMMENT '设备名称',
    device_address INT COMMENT '设备地址',
    temperature DECIMAL(10,4) COMMENT '温度',
    water_activity DECIMAL(10,4) COMMENT '水活性',
    ppm DECIMAL(10,4) COMMENT '含水量',
    viscosity DECIMAL(10,4) COMMENT '粘度',
    density DECIMAL(10,4) COMMENT '密度',
    dielectric_constant DECIMAL(10,4) COMMENT '介电常数',
    create_time BIGINT COMMENT '时间',
    index idx_osm_create_time(create_time)
);

CREATE TABLE IF NOT EXISTS oil_sensor_metrics (
    id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
    device_name VARCHAR(127) COMMENT '设备名称',
    device_address INT COMMENT '设备地址',
    temperature DECIMAL(10,4) COMMENT '温度',
    water_activity DECIMAL(10,4) COMMENT '水活性',
    ppm DECIMAL(10,4) COMMENT '含水量',
    viscosity DECIMAL(10,4) COMMENT '粘度',
    density DECIMAL(10,4) COMMENT '密度',
    dielectric_constant DECIMAL(10,4) COMMENT '介电常数',

    flags CHAR(16) COMMENT '标识',
    public_packet_time BIGINT COMMENT '公共报文时间',
    speed DECIMAL(10, 4) COMMENT '速度（KM/h）',
    km_post INT COMMENT '公里标（KM）',
    train_serial_no INT COMMENT '列号',
    temperature_outside INT COMMENT '外温',
    locomotive_double_heading_status INT COMMENT '列车联挂状态',
    carriage_no INT COMMENT '车厢号',
    train_no CHAR(16) COMMENT '车次号',

    create_time BIGINT COMMENT '时间',
    index idx_osm_create_time(create_time)
);

CREATE TABLE IF NOT EXISTS collect_module_metrics (
    id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
    device_name VARCHAR(127) COMMENT '设备名称',
    device_address INT COMMENT '设备地址',
    abrasion DECIMAL(10,4) COMMENT '行车磨耗',
    temperature DECIMAL(10,4) COMMENT '驻车温度',
    pressure DECIMAL(10,4) COMMENT '驻车压力',
    board_temperature DECIMAL(10, 4) COMMENT '板载温度',
    error_code INT COMMENT '故障码',
    create_time BIGINT COMMENT '时间',
    index idx_cmm_create_time(create_time)
);

CREATE TABLE IF NOT EXISTS public_info (
    id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
    packet_time DATETIME COMMENT '数据帧时间',
    packet_seq INT COMMENT '数据帧序列号',
    train_serial_no VARCHAR(64) COMMENT '列号',
    train_no VARCHAR(32) COMMENT '车次',
    couch_no INT COMMENT '车厢号',
    ccu_valid_flag INT COMMENT 'CCU有效标识',
    speed DECIMAL(10, 4) COMMENT '速度(km/h)',
    km_post DECIMAL(10, 4) COMMENT '公里标',
    longitude DECIMAL(10, 6) COMMENT '经度',
    latitude DECIMAL(10, 6) COMMENT '纬度',
    temperature DECIMAL(10, 4) COMMENT '温度',
    trunk_1_diameter DECIMAL(10, 4) COMMENT '1架轮径',
    trunk_2_diameter DECIMAL(10, 4) COMMENT '2架轮径',
    air_spring_1_pressure DECIMAL(10, 4) COMMENT '空簧压力1(kpa)',
    air_spring_2_pressure DECIMAL(10, 4) COMMENT '空簧压力2(kpa)',
    used_end INT COMMENT '占用端',
    create_time BIGINT COMMENT '时间',
    index idx_pi_create_time(create_time)
);

