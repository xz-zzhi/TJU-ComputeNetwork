chmod +x CGI/login.py
chmod +x CGI/register.py
加入权限

1. 在系统级环境中安装 pymysql
直接用 pip（或你的包管理器）把库安装到系统 Python：

bash
复制
编辑
# 对应 /usr/bin/python3
sudo /usr/bin/python3 -m pip install pymysql
或者，如果系统默认 python3 就是你 CGI 用的解释器：

bash
复制
编辑
sudo pip3 install pymysql
安装完成后，再跑 CGI 就能 import pymysql 了。

加入包


1. 验证你的数据库环境
在 VM 里执行：

bash
复制
编辑
# 看看有没有 mysql 或 mariadb 服务
systemctl status mysql
systemctl status mariadb

# 或尝试用 CLI 客户端连本地
mysql -u root -p -e "SHOW DATABASES;"
如果都报“command not found” 或 “Connection refused”，说明确实没装 MySQL。

2. 安装并启动一个本地 MySQL/MariaDB
下面以 Debian/Ubuntu 为例：

bash
复制
编辑
sudo apt update
sudo apt install mysql-server   # 或者 mariadb-server
sudo systemctl enable --now mysql.service
然后用 root 初始化一下：

bash
复制
编辑
sudo mysql_secure_installation
follow 提示设置 root 密码、禁用匿名、移除测试库等。

3. 在本地创建你的 DB、用户和表
进入 MySQL 控制台：

bash
复制
编辑
sudo mysql -u root -p
执行：

sql
复制
编辑
CREATE DATABASE socket_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER 'twt_vote'@'localhost' IDENTIFIED BY 'TWT_vote1895';
GRANT ALL PRIVILEGES ON socket_db.* TO 'twt_vote'@'localhost';
FLUSH PRIVILEGES;

USE socket_db;
CREATE TABLE users (
  u_id INT AUTO_INCREMENT PRIMARY KEY,
  u_name VARCHAR(64) NOT NULL UNIQUE,
  u_pass VARCHAR(128) NOT NULL,
  ip VARCHAR(45),
  created_at DATETIME,
  updated_at DATETIME
);
这样你的脚本中所用的 pymysql.connect(..., host="localhost", user="twt_vote", ...) 就能顺利连上。

4. 修改你的 Python 脚本连接本地
把原来

python
复制
编辑
db = pymysql.connect(host="8.141.166.181", user="twt_vote", password="TWT_vote1895", database="socket_db")
改成

python
复制
编辑
db = pymysql.connect(
    host="localhost",
    user="twt_vote",
    password="TWT_vote1895",
    database="socket_db",
    connect_timeout=3,
    charset="utf8mb4",
)