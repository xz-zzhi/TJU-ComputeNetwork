#!/usr/bin/env python3
# encoding:utf-8
#
#   This script does not provide a valid CGI response or a valid HTTP/1.1
#   response.  In fact, it does not even produce valid HTML.  It is meant to
#   help you investigate the variables and other parameters your server is
#   passing to the script.  You may want to turn the prints into writes to a
#   a log file especially when you start redirecting stdout of the script.
#
#   Authors: Athula Balachandran <abalacha@cs.cmu.edu>
#            Charles Rang <rang@cs.cmu.edu>
#            Wolfgang Richter <wolf@cs.cmu.edu>

from os import environ
import sqlite3
import time
import json
import sys

# 调试：把几个关键 env 打到 stderr
print(f"[DEBUG] QUERY_STRING={environ.get('QUERY_STRING')}", file=sys.stderr)
print(f"[DEBUG] REMOTE_ADDR  ={environ.get('REMOTE_ADDR')}",  file=sys.stderr)

# *** 给浏览器的 HTTP 头 ***
print("Content-Type: application/json; charset=utf-8")
print()  # <- 关键的空行，分隔头和 body

# 解析 GET 参数
qstring = environ.get("QUERY_STRING","")
uName = qstring[qstring.find('=')+1:qstring.find('&')] if "&" in qstring else ""
rest   = qstring[qstring.find('&')+1:] if "&" in qstring else ""
uPass  = rest[rest.find('=')+1:] if "=" in rest else ""
uIp    = environ.get("REMOTE_ADDR","")
method = environ.get("REQUEST_METHOD","")
Response = {}
dic = {}

dic['Ip']     = uIp
dic['method'] = method

if len(uName)==0 or len(uPass)==0:
    dic['Ip']     = uIp
    dic['method'] = method
    Response['Code']    = "400"
    Response['Msg']     = "Invalid User name or Password"
    Response['Results'] = dic
    print(json.dumps(Response))
    sys.exit(0)

# 创建／打开 SQLite3 数据库
db = sqlite3.connect("zzhi.db", timeout=3)
cursor = db.cursor()
# 建表（第一次运行时有效）
cursor.execute("""
CREATE TABLE IF NOT EXISTS users (
  u_id        INTEGER PRIMARY KEY AUTOINCREMENT,
  u_name      TEXT UNIQUE,
  u_pass      TEXT,
  ip          TEXT,
  created_at  TEXT,
  updated_at  TEXT
)
""")
db.commit()

# 检查是否已注册
cursor.execute("SELECT u_pass FROM users WHERE u_name=?", (uName,))
row = cursor.fetchone()
if row:
    # 已注册，拒绝重复注册
    Response['Code']    = "400"
    Response['Msg']     = "User already registered"
    Response['Results'] = dic
else:
    # 插入新用户
    now = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
    try:
        cursor.execute(
            "INSERT INTO users (u_name, u_pass, ip, created_at, updated_at) VALUES (?, ?, ?, ?, ?)",
            (uName, uPass, uIp, now, now)
        )
        db.commit()
        uid = cursor.lastrowid
        Response['Code']    = "200"
        Response['Msg']     = "You are now registered"
        Response['Results'] = {
            "u_id":        uid,
            "name":        uName,
            "pass":        uPass,
            "created_at":  now,
            "updated_at":  now
        }
    except Exception as e:
        db.rollback()
        Response['Code']    = "400"
        Response['Msg']     = "SQL Insert Error"
        Response['Results'] = dic

db.close()
print(json.dumps(Response))
