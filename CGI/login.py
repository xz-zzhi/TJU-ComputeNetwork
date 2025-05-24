#!/usr/bin/env python3
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
    Response['Code']    = "400"
    Response['Msg']     = "Invalid User Name or User Password"
    Response['Results'] = dic
    print(json.dumps(Response))
    sys.exit(0)

# 打开 SQLite3 数据库
db = sqlite3.connect("zzhi.db", timeout=3)
cursor = db.cursor()

# 查询用户
cursor.execute("SELECT u_id, u_pass, ip, created_at, updated_at FROM users WHERE u_name=?", (uName,))
row = cursor.fetchone()
if row:
    uid, db_pass, last_ip, created_at, updated_at = row
    # 密码校验
    if db_pass != uPass:
        Response['Code'] = "400"
        Response['Msg']  = "Password Error"
    else:
        now = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
        # 更新 ip 和 updated_at
        try:
            cursor.execute(
                "UPDATE users SET ip=?, updated_at=? WHERE u_name=?",
                (uIp, now, uName)
            )
            db.commit()
        except:
            db.rollback()
        Response['Code'] = "200"
        Response['Msg']  = "Login successful"
        Response['Results'] = {
            "u_id":        uid,
            "name":        uName,
            "last_ip":     last_ip,
            "created_at":  created_at,
            "updated_at":  now
        }
else:
    Response['Code'] = "400"
    Response['Msg']  = "You have to register before login"

db.close()
# 保留原始输出结构
if 'Results' not in Response:
    Response['Results'] = dic

print(json.dumps(Response))
