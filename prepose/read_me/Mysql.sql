-- 创建数据库
drop database IF EXISTS PEP;
create database PEP;
use PEP;


-- CURRVAL 实现方案
DROP FUNCTION IF EXISTS currval;
DELIMITER $
CREATE FUNCTION currval (seq_name VARCHAR(50))
RETURNS INTEGER
CONTAINS SQL
BEGIN
  DECLARE value INTEGER;
  SET value = 0;
  SELECT current_value INTO value
  FROM xpep_jnls_trace_sequence
  WHERE name = seq_name;
  RETURN value;
END$
DELIMITER ;

-- nextval 实现方案
DROP FUNCTION IF EXISTS nextval;
DELIMITER $
CREATE FUNCTION nextval (seq_name VARCHAR(50))
RETURNS INTEGER
CONTAINS SQL
BEGIN
   UPDATE xpep_jnls_trace_sequence
   SET          current_value = current_value + increment
   WHERE name = seq_name;
   RETURN currval(seq_name);
END$
DELIMITER ;

-- setval 实现方案
DROP FUNCTION IF EXISTS setval;
DELIMITER $
CREATE FUNCTION setval (seq_name VARCHAR(50), value INTEGER)
RETURNS INTEGER
CONTAINS SQL
BEGIN
   UPDATE xpep_jnls_trace_sequence
   SET          current_value = value
   WHERE name = seq_name;
   RETURN currval(seq_name);
END$
DELIMITER ;

-- 用法实例
-- 查询当前trace_seq的值
-- select currval('trace_seq'); 

-- 查询下一个trace_seq的值
-- select nextval('trace_seq');  

-- 设置当前trace_seq的值
-- select setval('trace_seq',150);  




-- 商户黑名单表
drop table IF EXISTS BLACKMER_INFO;
create table BLACKMER_INFO
(
  mercode     CHAR(15) not null,
  termcde     CHAR(8) default '00000000',
  adddate     CHAR(8) not null,
  enddate     CHAR(8) not null,
  trancdelist CHAR(127) default '*'
)DEFAULT CHARSET=utf8;
alter table BLACKMER_INFO
  add constraint BLACKMER_INFO_pk primary key (mercode);

insert into BLACKMER_INFO(mercode, termcde, adddate, enddate, trancdelist)values('860010030210001','12345678','20120604','20120607','*,101');
insert into BLACKMER_INFO(mercode, termcde, adddate, enddate)values('860010030210002','12345678','20120604','20120607');
insert into BLACKMER_INFO(mercode, termcde, adddate, enddate, trancdelist)values('860010030210003','12345678','20120604','20120607','*,101,102,103,104,105');
commit;

-- Create table
drop table IF EXISTS CARD_TRANSFER;
create table CARD_TRANSFER
(
  card_id                INTEGER not null,
  card_ins_desc          VARCHAR(64) not null,
  card_ins_code          VARCHAR(32) not null,
  card_desc              VARCHAR(64),
  card_atm               INTEGER,
  card_pos               INTEGER,
  card_tracktwo          VARCHAR(4),
  card_trackthree        VARCHAR(4),
  card_tracktwo_length   INTEGER, 
  card_trackthree_length INTEGER, 
  card_no_length         INTEGER not null,
  card_length            INTEGER not null,
  card_head              VARCHAR(20) not null,
  card_type_desc         VARCHAR(64),
  card_type              VARCHAR(5)   
)DEFAULT CHARSET=utf8;
-- Create/Recreate primary, unique and foreign key constraints 
alter table CARD_TRANSFER
  add primary key (CARD_ID);

-- Create/Recreate indexes 
create index CARD_TRANSFER_INDEX on CARD_TRANSFER (CARD_HEAD, CARD_NO_LENGTH);
commit;

-- 保存EWP发送报文信息中所需要的信息
drop table IF EXISTS EWP_INFO;
create table EWP_INFO
(
  transtype      VARCHAR(4) not null,
  transdealcode  VARCHAR(6) not null,
  sentdate       VARCHAR(8) not null,
  senttime       VARCHAR(6) not null,
  sentinstitu    VARCHAR(4),
  transcode      VARCHAR(3) not null,
  orderno        VARCHAR(20),
  psamid         VARCHAR(16) not null,
  translaunchway VARCHAR(1),
  selfdefine     VARCHAR(20),
  incdname       VARCHAR(50),
  incdbkno       VARCHAR(25),
  transfee       VARCHAR(12)
)DEFAULT CHARSET=utf8;
-- Create/Recreate primary, unique and foreign key constraints 
alter table EWP_INFO
  add constraint EWP_JNLS_PK primary key (SENTDATE, SENTTIME, transdealcode, psamid);
  
-- Create Index
create index EWP_INFO_index1 on EWP_INFO (SENTDATE, SENTTIME, ORDERNO, psamid);

commit;

-- 交易配置信息表
drop table IF EXISTS MSGCHG_INFO;

create table MSGCHG_INFO
(
  trancde  CHAR(3) not null,
  msgtype  CHAR(4) not null,
  process  CHAR(6) not null,
  service  CHAR(2) not null,
  amtcode  CHAR(3) not null, 
  mercode  CHAR(15) not null,
  termcde  CHAR(8) not null,
  subtype  CHAR(4),          
  revoid   CHAR(1),
  bit_20   CHAR(4),
  bit_25   CHAR(2),
  fsttran  CHAR(40),
  remark   CHAR(40) not null,
  flag20   CHAR(1),
  flagpe   CHAR(1), 
  cdlimit  CHAR(1), 
  amtlimit CHAR(12),
  amtmin   CHAR(12),
  flagpwd  CHAR(1), 
  flagfee  CHAR(1) 
)DEFAULT CHARSET=utf8;
-- Create/Recreate indexes 
create index MSGCHGINFOmercode_IDX on MSGCHG_INFO (mercode);

-- Create/Recreate primary, unique and foreign key constraints 
alter table MSGCHG_INFO
  add constraint MSGCHG_INFO_PK primary key (trancde);
  
insert into MSGCHG_INFO(trancde, msgtype, process, service, amtcode, mercode, termcde, revoid, remark, amtmin, flagpwd, flagpe) values('101','0200','910000','00','156','860010030210001','12345678','0','网票网电影票订单支付','0','1','Y');
insert into MSGCHG_INFO(trancde, msgtype, process, service, amtcode, mercode, termcde, revoid, remark, amtmin, flagpwd, flagpe) values('102','0200','910000','00','156','860010030210002','12345678','0','融易通航服机票订单支付','0','1','Y');
insert into MSGCHG_INFO(trancde, msgtype, process, service, amtcode, mercode, termcde, revoid, remark, amtmin, flagpwd, flagpe) values('103','0200','910000','00','156','860010030210003','12345678','0','东航航空机票订单支付','0','1','Y');
insert into MSGCHG_INFO(trancde, msgtype, process, service, amtcode, mercode, termcde, revoid, remark, amtmin, flagpwd, flagpe) values('104','0200','910000','00','156','860010030210004','12345678','0','新泛联数码游戏充值订单支付','0','1','Y');
insert into MSGCHG_INFO(trancde, msgtype, process, service, amtcode, mercode, termcde, revoid, remark, amtmin, flagpwd, flagpe) values('105','0200','910000','00','156','860010030210005','12345678','0','汇元网游戏点卡订单支付','0','1','Y');
insert into MSGCHG_INFO(trancde, msgtype, process, service, amtcode, mercode, termcde, revoid, remark, amtmin, flagpwd, flagpe) values('106','0200','910000','00','156','860010030210006','12345678','0','交通罚款订单支付','0','1','Y');
insert into MSGCHG_INFO(trancde, msgtype, process, service, amtcode, mercode, termcde, revoid, remark, amtmin, flagpwd, flagpe) values('107','0200','910000','00','156','860010030210007','12345678','0','水电煤订单支付','0','1','Y');
commit;

-- 前置系统交易记录表
drop table IF EXISTS PEP_JNLS;
create table PEP_JNLS
(
  pepdate 	     CHAR(8) not null,  -- 前置系统的日期 报文第7域
  peptime        CHAR(6) not null,  -- 前置系统的时间 报文第7域
  trancde        CHAR(3) not null,  -- 交易码
  msgtype        CHAR(4) not null,  -- 消息类型
  outcdno        VARCHAR(20),          -- 转出卡号
  outcdtype      VARCHAR(1),           -- 转出卡类型
  intcdno        VARCHAR(40),          -- 转入卡号
  intcdbank 	 VARCHAR(30),	-- 保存转账汇款交易的转卡号银行编号
  intcdname 	 VARCHAR(30),	-- 保存转账汇款交易的转入卡姓名
  process        CHAR(6) not null,  -- 交易处理码
  tranamt        VARCHAR(12),          -- 交易金额
  trace          numeric(6) not null, -- 前置系统流水
  termtrc        VARCHAR(6),            -- 终端流水
  trntime        CHAR(6) not null,   -- ewp过来的时间/pos过来的时间
  trndate        CHAR(8) not null,   -- ewp过来的日期/pos过来的日期
  poitcde        VARCHAR(3),
  track2         VARCHAR(37),          -- 二磁
  track3         VARCHAR(104),         -- 三磁
  sendcde        VARCHAR(8),			-- 终端标示码 01000000(手机),02000000(管理平台),03000000(POS),04000000(sk ewp)
  syseqno        VARCHAR(12),          -- 参考号
  termcde        VARCHAR(8) not null,  -- 内部终端号
  mercode        VARCHAR(15) not null, -- 内部商户号
  billmsg        VARCHAR(40) not null, -- 订单号/DY+日期(pepdate,8)+时间(peptime,6)+前置流水(trace,6)
  samid          VARCHAR(20) not null, -- psam卡号/POS机无该字段信息
  termid         VARCHAR(25),          -- 电话号码/APN卡号
  termidm        VARCHAR(20),			-- 终端物理编号,手机/POS机硬件编号
  gpsid			 VARCHAR(35),			-- 无线POS基站编码
  aprespn        VARCHAR(2),           -- 应答码
  sdrespn        VARCHAR(2),
  revodid        VARCHAR(1),
  billflg        VARCHAR(1),
  mmchgid        VARCHAR(1),
  mmchgnm        numeric(16),
  termmac        VARCHAR(16),
  termpro        VARCHAR(200),
  trnsndpid      numeric(4),			 -- Folder ID 
  trnsndp        VARCHAR(15),       -- folder名称
  trnmsgd        VARCHAR(4),        -- TPDU源地址
  revdate        VARCHAR(10),  
  modeflg        CHAR(1),            -- 是否发送短信的标示
  modecde        VARCHAR(24),       -- 用户接收短息的手机号码
  filed28        VARCHAR(9),            -- 交易手续费
  filed48        VARCHAR(256),
  merid          numeric(5),         -- vas平台的商户号
  authcode       VARCHAR(6),            -- 授权码
  discountamt    VARCHAR(12),      -- EWP系统优惠金额
  poscode        VARCHAR(2),
  translaunchway VARCHAR(1),
  batchno        VARCHAR(6),			 -- 交易批次号
  camtlimit      numeric(1),			 -- 信用卡日限额判断标识
  damtlimit      numeric(1),			 -- 借记卡日限额判断标识
  termmeramtlimit      numeric(1),			 -- 终端商户信用卡日限额判断标识
  settlementdate  VARCHAR(4),			-- 清算日期MMDD,报文15域
  reversedflag    VARCHAR(1),			-- 是否发生冲正的标识 1：终端发起冲正  0：未发起冲正
  reversedans     VARCHAR(2),			-- 冲正应答
  sysreno         VARCHAR(12),			-- 终端冲正返回的系统参考号
  postermcde      VARCHAR(8),			-- pos终端送上来的商户号
  posmercode      VARCHAR(15),			-- pos终端送上的终端号
  filed44 		VARCHAR(22) -- 附加相应数据在交易响应消息中中间会有空格的返回接收机构和收单机构的标识码
)DEFAULT CHARSET=utf8;

alter table PEP_JNLS
  add constraint PEP_JNLS_pk primary key (pepdate, peptime, trace, samid);

--  Create/Recreate indexes 
create index PEP_JNLS_BILLMSG on PEP_JNLS (BILLMSG, PEPDATE, PEPTIME, APRESPN);
create index PEP_JNLS_INDEX on PEP_JNLS (PEPDATE, PEPTIME, PROCESS, TRACE, TERMCDE, MERCODE);
create index PEP_JNLS_INDEX1 on PEP_JNLS (batchno, process, aprespn, termidm, reversedflag, reversedans);
create index PEP_JNLS_INDEX2 on PEP_JNLS (syseqno, aprespn, outcdno, mercode);
create index PEP_JNLS_INDEX3 on PEP_JNLS (posmercode, postermcde, batchno, aprespn, trancde, termidm, reversedflag, reversedans);
create index PEP_JNLS_INDEX4 on PEP_JNLS (posmercode, postermcde, batchno, termtrc, outcdno, tranamt, termidm);

--  Add/modify columns 
-- alter table PEP_JNLS add INTCDBANK VARCHAR(30);	-- 保存转账汇款交易的转卡号银行编号
-- alter table PEP_JNLS add INTCDNAME VARCHAR(30);	-- 保存转账汇款交易的转入卡姓名

commit;

drop table IF EXISTS PEP_JNLS_ALL;
create table PEP_JNLS_ALL
(
  pepdate        CHAR(8) not null,  -- 前置系统的日期 报文第7域
  peptime        CHAR(6) not null,  -- 前置系统的时间 报文第7域
  trancde        CHAR(3) not null,  -- 交易码
  msgtype        CHAR(4) not null,  -- 消息类型
  outcdno        VARCHAR(20),          -- 转出卡号
  outcdtype      VARCHAR(1),           -- 转出卡类型
  intcdno        VARCHAR(40),          -- 转入卡号
  intcdbank    VARCHAR(30), -- 保存转账汇款交易的转卡号银行编号
  intcdname    VARCHAR(30), -- 保存转账汇款交易的转入卡姓名
  process        CHAR(6) not null,  -- 交易处理码
  tranamt        VARCHAR(12),          -- 交易金额
  trace          numeric(6) not null, -- 前置系统流水
  termtrc        VARCHAR(6),            -- 终端流水
  trntime        CHAR(6) not null,   -- ewp过来的时间/pos过来的时间
  trndate        CHAR(8) not null,   -- ewp过来的日期/pos过来的日期
  poitcde        VARCHAR(3),
  track2         VARCHAR(37),          -- 二磁
  track3         VARCHAR(104),         -- 三磁
  sendcde        VARCHAR(8),      -- 终端标示码 01000000(手机),02000000(管理平台),03000000(POS),04000000(sk ewp)
  syseqno        VARCHAR(12),          -- 参考号
  termcde        VARCHAR(8) not null,  -- 内部终端号
  mercode        VARCHAR(15) not null, -- 内部商户号
  billmsg        VARCHAR(40) not null, -- 订单号/DY+日期(pepdate,8)+时间(peptime,6)+前置流水(trace,6)
  samid          VARCHAR(20) not null, -- psam卡号/POS机无该字段信息
  termid         VARCHAR(25),          -- 电话号码/APN卡号
  termidm        VARCHAR(20),     -- 终端物理编号,手机/POS机硬件编号
  gpsid      VARCHAR(35),     -- 无线POS基站编码
  aprespn        VARCHAR(2),           -- 应答码
  sdrespn        VARCHAR(2),
  revodid        VARCHAR(1),
  billflg        VARCHAR(1),
  mmchgid        VARCHAR(1),
  mmchgnm        numeric(16),
  termmac        VARCHAR(16),
  termpro        VARCHAR(200),
  trnsndpid      numeric(4),       -- Folder ID 
  trnsndp        VARCHAR(15),       -- folder名称
  trnmsgd        VARCHAR(4),        -- TPDU源地址
  revdate        VARCHAR(10),  
  modeflg        CHAR(1),            -- 是否发送短信的标示
  modecde        VARCHAR(24),       -- 用户接收短息的手机号码
  filed28        VARCHAR(9),            -- 交易手续费
  filed48        VARCHAR(256),
  merid          numeric(5),         -- vas平台的商户号
  authcode       VARCHAR(6),            -- 授权码
  discountamt    VARCHAR(12),      -- EWP系统优惠金额
  poscode        VARCHAR(2),
  translaunchway VARCHAR(1),
  batchno        VARCHAR(6),       -- 交易批次号
  camtlimit      numeric(1),       -- 信用卡日限额判断标识
  damtlimit      numeric(1),       -- 借记卡日限额判断标识
  termmeramtlimit      numeric(1),       -- 终端商户信用卡日限额判断标识
  settlementdate  VARCHAR(4),     -- 清算日期MMDD,报文15域
  reversedflag    VARCHAR(1),     -- 是否发生冲正的标识 1：终端发起冲正  0：未发起冲正
  reversedans     VARCHAR(2),     -- 冲正应答
  sysreno         VARCHAR(12),      -- 终端冲正返回的系统参考号
  postermcde      VARCHAR(8),     -- pos终端送上来的商户号
  posmercode      VARCHAR(15),      -- pos终端送上的终端号
  filed44     VARCHAR(22) -- 附加相应数据在交易响应消息中中间会有空格的返回接收机构和收单机构的标识码
)DEFAULT CHARSET=utf8;

alter table PEP_JNLS_ALL
  add constraint PEP_JNLS_ALL_pk primary key (pepdate, peptime, trace, samid);

--  Create/Recreate indexes 
create index PEP_JNLS_BILLMSG on PEP_JNLS (BILLMSG, PEPDATE, PEPTIME, APRESPN);
create index PEP_JNLS_INDEX on PEP_JNLS (PEPDATE, PEPTIME, PROCESS, TRACE, TERMCDE, MERCODE);
create index PEP_JNLS_INDEX1 on PEP_JNLS (batchno, process, aprespn, termidm, reversedflag, reversedans);
create index PEP_JNLS_INDEX2 on PEP_JNLS (syseqno, aprespn, outcdno, mercode);
create index PEP_JNLS_INDEX3 on PEP_JNLS (posmercode, postermcde, batchno, aprespn, trancde, termidm, reversedflag, reversedans);
create index PEP_JNLS_INDEX4 on PEP_JNLS (posmercode, postermcde, batchno, termtrc, outcdno, tranamt, termidm);

--  Add/modify columns 
-- alter table PEP_JNLS add INTCDBANK VARCHAR(30);  -- 保存转账汇款交易的转卡号银行编号
-- alter table PEP_JNLS add INTCDNAME VARCHAR(30);  -- 保存转账汇款交易的转入卡姓名

commit;

create table PEP_JNLS_ALL
(
  pepdate         CHAR(8) not null,
  peptime         CHAR(6) not null,
  trancde         CHAR(3) not null,
  msgtype         CHAR(4) not null,
  outcdno         CHAR(20),
  outcdtype       CHAR(1),
  intcdno         CHAR(40),
  process         CHAR(6) not null,
  tranamt         CHAR(12),
  trace           numeric(6) not null,
  termtrc         CHAR(6),
  trntime         CHAR(6) not null,
  trndate         CHAR(8) not null,
  poitcde         CHAR(3),
  track2          CHAR(37),
  track3          CHAR(104),
  sendcde         CHAR(8),
  syseqno         CHAR(12),
  termcde         CHAR(8) not null,
  mercode         CHAR(15) not null,
  billmsg         CHAR(40),
  samid           CHAR(23) not null,
  termid          CHAR(25),
  termidm         CHAR(20),
  aprespn         CHAR(2),
  sdrespn         CHAR(2),
  revodid         CHAR(1),
  billflg         CHAR(1),
  mmchgid         CHAR(1),
  mmchgnm         numeric(16),
  termmac         CHAR(16),
  termpro         VARCHAR(200),
  trnsndpid       numeric(4),
  trnsndp         VARCHAR(15),
  trnmsgd         VARCHAR(4),
  revdate         CHAR(10),
  modeflg         CHAR(1),
  modecde         CHAR(24),
  filed28         CHAR(9),
  filed48         VARCHAR(256),
  merid           numeric(5),
  authcode        CHAR(6),
  discountamt     CHAR(12),
  poscode         CHAR(2),
  translaunchway  CHAR(1),
  batchno         CHAR(6),
  camtlimit       numeric(1),
  damtlimit       numeric(1),
  termmeramtlimit numeric(1),
  settlementdate  CHAR(4),
  reversedflag    CHAR(1),
  reversedans     CHAR(2),
  sysreno         CHAR(12),
  postermcde      CHAR(8),
  posmercode      CHAR(15),
  filed44         CHAR(22),
  gpsid           CHAR(35),
  intcdbank       VARCHAR(30),
  intcdname       VARCHAR(30)
);
-- Create/Recreate indexes 
create index PEP_JNLS_ALL_INDEX1 on PEP_JNLS_ALL (BATCHNO, PROCESS, APRESPN, TERMIDM, REVERSEDFLAG, REVERSEDANS);
create index PEP_JNLS_ALL_INDEX2 on PEP_JNLS_ALL (SYSEQNO, APRESPN, OUTCDNO, MERCODE, TRANCDE);
create index PEP_JNLS_ALL_INDEX3 on PEP_JNLS_ALL (POSMERCODE, POSTERMCDE, BATCHNO, APRESPN, TRANCDE, TERMIDM, REVERSEDFLAG, REVERSEDANS);
create index PEP_JNLS_ALL_INDEX4 on PEP_JNLS_ALL (POSMERCODE, POSTERMCDE, BATCHNO, TERMTRC, OUTCDNO, TRANAMT, TERMIDM);
-- Create/Recreate primary, unique and foreign key constraints 
alter table PEP_JNLS_ALL
  add constraint PEP_JNLS_ALL_PK primary key (PEPDATE, PEPTIME, TRACE);


-- pos批上送表明细
drop table IF EXISTS pos_batch_send_detail_jnls;
create table pos_batch_send_detail_jnls
(
  id				   int not null, -- 主键id 自增长
  pid				   int not null, -- 来自pos_settlement_detail_jnls表的外键约束
  trace				   numeric(6) not null,  -- 流水
  cardno			   VARCHAR(19),  -- 银行卡号
  amt				   VARCHAR(12),  -- 交易金额
  reference			   VARCHAR(12),  -- 参考号
  pos_mercode          VARCHAR(15) not null,  -- 内部商户号
  pos_termcode         VARCHAR(8) not null, -- 内部终端号
  batch_no             numeric(6) not null,	-- 批次号
  reslut			   VARCHAR(2) not null		-- 应答码,39域
)DEFAULT CHARSET=utf8;

alter table pos_batch_send_detail_jnls add constraint pos_batch_send_detail_jnls_pk primary key (id);

-- create sequence pos_batchsend_detail_sequence
-- minvalue 1
-- maxvalue 999999999
-- start with 1
-- increment by 1
-- cache 50;

-- pos批上送表明细序列号
DROP TABLE IF EXISTS pos_batchsend_detail_sequence;
CREATE TABLE pos_batchsend_detail_sequence (
    name              VARCHAR(50) NOT NULL,
    current_value INT NOT NULL,
    increment       INT NOT NULL DEFAULT 1,
    PRIMARY KEY (name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
INSERT INTO pos_batchsend_detail_sequence VALUES ('trace_detail_seq',1,1);

commit;

-- POS_CHANNEL_INFO 表
drop table IF EXISTS POS_CHANNEL_INFO;
create table POS_CHANNEL_INFO
(
  mgr_term_id VARCHAR(8) not null, -- pos终端号
  mgr_mer_id  VARCHAR(15) not null, -- pos商户号
  channel_id  numeric(4) not null  -- 终端pos所属渠道ID
)DEFAULT CHARSET=utf8;
--  Create/Recreate primary, unique and foreign key constraints 
alter table POS_CHANNEL_INFO
  add constraint POS_CHANNEL_INFOPK primary key (MGR_TERM_ID, MGR_MER_ID, CHANNEL_ID);
  
commit;

-- pos机的配置信息表
drop table IF EXISTS POS_CONF_INFO;
create table POS_CONF_INFO
(
  mgr_term_id      VARCHAR(8) not null,  -- 终端号
  mgr_mer_id       VARCHAR(15) not null, -- 商户号
  mgr_flag         VARCHAR(1),  -- 商户标识，签到是否成功标示，Y：签到成功，N：签到失败
  mgr_mer_des      VARCHAR(40), -- 商户名称
  mgr_batch_no     VARCHAR(6),	-- 批次号
  mgr_folder       VARCHAR(14), -- 链路名称
  mgr_tpdu         VARCHAR(22), -- TPDU头
  mgr_signout_flag numeric(1), -- 签退标示，1—自动，0—不自动
  mgr_key_index    numeric(4), -- 当前使用主密钥索引值
  mgr_kek          VARCHAR(32), -- kek
  mgr_desflg       VARCHAR(3), -- 加密方法：DES：001 3DES：003  磁道密钥：004
  mgr_pik          VARCHAR(32), -- pik工作密钥
  mgr_mak          VARCHAR(16), -- mak工作密钥
  mgr_tdk          VARCHAR(32), -- 磁道工作密钥
  mgr_signtime     VARCHAR(14),  -- YYYYMMDD hhmmss
  track_pwd_flag   VARCHAR(2),  -- 是否有磁道、密码的标记  00 无磁无密 01 无磁有密  10 有磁无密  11 有磁有密
  pos_machine_id   VARCHAR(20) not null,  -- pos 机具编号
  pos_machine_type int not null, -- 2:个人移动支付终端3:有线pos,4:无线gprspos,5:无线apn卡pos,6:sk项目POS(一体机),7:USB POS(车载项目)
  pos_apn          VARCHAR(30), -- apn卡号
  pos_telno        VARCHAR(16), -- 电话号码
  pos_gpsno        VARCHAR(35),  -- 定位号码
  pos_type		   VARCHAR(2), --  pos终端应用类型默认值60
  cz_retry_count   numeric(1),  -- 冲正重发次数 默认值3次
  trans_telno1	   VARCHAR(14),  -- 三个交易电话号码之一,14位数字,右补空格
  trans_telno2	   VARCHAR(14),  -- 三个交易电话号码之二,14位数字,右补空格
  trans_telno3	   VARCHAR(14),  -- 三个交易电话号码之三,14位数字,右补空格
  manager_telno	   VARCHAR(14),  -- 一个管理电话号码,14位数字
  hand_flag		   numeric(1),  -- 是否支持手工输入卡号 1:支持,0:不支持(默认不支持)	
  trans_retry_count   numeric(1),  -- 交易重发次数  默认3次
  pos_main_key_index  VARCHAR(2),  -- 终端主密钥索引值 默认为01
  pos_ip_port1     VARCHAR(30),  -- 无线POS IP1 和端口号1
  pos_ip_port2     VARCHAR(30),  -- 无线POS IP2 和端口号2
  rtn_track_pwd_flag  VARCHAR(2), -- 消费退货是否需要磁道和密码 10:有磁道无密码
  mgr_timeout   numeric(2), -- 超时时间 默认60秒
  input_order_flg VARCHAR(2),-- 终端是否有订单号输入界面的标记.01:终端存在输入订单号界面 00:终端不存在输入订单号的界面默认是00
  pos_update_flag    numeric(1) default 0,
  pos_update_add     VARCHAR(30) -- POS应用程序更新的地址IP和端口
)DEFAULT CHARSET=utf8;

--  Create/Recreate primary, unique and foreign key constraints 
alter table POS_CONF_INFO
  add constraint POSCONFINFO_PK primary key (MGR_TERM_ID, MGR_MER_ID, POS_MACHINE_ID);
commit;

-- 智能终端个人收单商户配置信息表
drop table IF EXISTS pos_cust_info;
create table pos_cust_info
(
  cust_id      		  VARCHAR(20) not null,  -- 用户号码，比如手机号码等
  cust_psam			  VARCHAR(16) not null,  -- psam卡号
  cust_merid          VARCHAR(15) not null, -- 内部商户号
  cust_termid         VARCHAR(8) not null,  -- 内部终端号
  cust_incard         VARCHAR(23) not null, -- 转入卡号
  cust_ebicode        VARCHAR(20) not null, -- 电银帐号
  cust_merdescr       VARCHAR(50) not null,	-- 商户名称
  cust_merdescr_utf8  VARCHAR(200) not null,		-- 商户名称的UTF_8的格式
  cust_coraccountno	  VARCHAR(23), -- 对公账户号
  cust_updatetime     timestamp(6)   -- YYYY-MM-DD HH24:MI:SS
)DEFAULT CHARSET=utf8;
alter table pos_cust_info add constraint POS_CUST_INFO_PK primary key (CUST_PSAM);
alter table pos_cust_info add constraint POS_CUST_ID_UQ unique (CUST_ID);

-- pos批上送表
drop table IF EXISTS pos_settlement_detail_jnls;
create table pos_settlement_detail_jnls
(
  id				   int not null, -- 主键id
  pid				   int not null, -- 来自 表pos_settlement_jnls的外键约束
  trace				   numeric(6) not null, -- 流水
  reference			   VARCHAR(12) not null,  -- 参考号
  pos_mercode          VARCHAR(15) not null,  -- 内部商户号
  pos_termcode         VARCHAR(8) not null, -- 内部终端号
  batch_no             numeric(6) not null,	-- 批次号 60.2
  reslut			   VARCHAR(2) not null,		-- 应答码,39域
  netmanager_code		VARCHAR(3) not null,		-- 网络管理信息码，60.3 
  package_count	       int not null, -- 当前报文包含交易总笔数 ，48域中包含交易的笔数或存批上送结束时，批上送的总笔数
  pep_jnls_count	   int, -- 实际查询获得的交易总笔数
  success_count	       int, -- 实际成功总笔数
  failed_count	       int, -- 实际失败总笔数 
  bit48	               VARCHAR(322), -- 批上送报文48域原始信息
  senddate      	   VARCHAR(8) not null,  -- 前置日期
  sendtime             VARCHAR(6) not null -- 前置时间
)DEFAULT CHARSET=utf8;


--  create sequence pos_batch_send_sequence
--  minvalue 1
--  maxvalue 999999999
--  start with 1
--  increment by 1
--  cache 50;

alter table pos_settlement_detail_jnls add constraint pos_settlement_detail_jnls_pk primary key (id);

-- pos批结算表
drop table IF EXISTS pos_settlement_jnls;
create table pos_settlement_jnls
(
  id				   int not null,    -- 主键，自增长
  xpepdate      	   VARCHAR(8) not null,  -- 前置日期
  xpeptime             VARCHAR(6) not null, -- 前置时间
  trace				   numeric(6) not null,
  reference			   VARCHAR(12) not null,  -- 参考号
  pos_mercode          VARCHAR(15) not null,  -- 内部商户号
  pos_termcode         VARCHAR(8) not null, -- 内部终端号
  settlement_date      VARCHAR(4) not null, -- 清算日期
  batch_no             numeric(6) not null,	-- 批次号
  operator_no		   VARCHAR(3) not null,		-- 操作员代码
  pos_jieji_amount	   VARCHAR(12), -- 借记总金额 
  pos_jieji_count	   int, -- 借记总笔数 
  pos_daiji_amount	   VARCHAR(12), -- 贷记总金额 
  pos_daiji_count	   int, -- 贷记总笔数 
  xpep_jieji_amount	   VARCHAR(12), -- xpep借记总金额 
  xpep_jieji_count	   int, -- xpep借记总笔数 
  xpep_daiji_cx_amount	   VARCHAR(12), -- xpep贷记(撤销)总金额 
  xpep_daiji_cx_count	   int, -- xpep贷记(撤销)总笔数
  xpep_daiji_th_amount	   VARCHAR(12), -- xpep贷记(退货)总金额 
  xpep_daiji_th_count	   int, -- xpep贷记(退货)总笔数
  settlement_result	   int,  -- 对账应答码
  termidm   	VARCHAR(20)	-- 终端物理编号,手机/POS机硬件编号 机具编号
)DEFAULT CHARSET=utf8;

-- create sequence pos_settlement_jnls_sequence
-- minvalue 1
-- maxvalue 999999999
-- start with 1
-- increment by 1
-- cache 50;

-- alter table pos_settlement_jnls add constraint pos_settlement_jnls_ primary key (xpepdate,xpeptime,trace,pos_mercode,pos_termcode);
alter table pos_settlement_jnls add constraint pos_settlement_jnls_pk primary key (id);

-- pos批结算表序列号
DROP TABLE IF EXISTS pos_settlement_jnls_sequence;
CREATE TABLE pos_settlement_jnls_sequence (
    name              VARCHAR(50) NOT NULL,
    current_value INT NOT NULL,
    increment       INT NOT NULL DEFAULT 1,
    PRIMARY KEY (name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
INSERT INTO pos_settlement_jnls_sequence VALUES ('trace_settlement_seq',1,1);


-- 终端投资方信息表，比如上海电信就是一个终端投资方。
-- 终端投资方是指该psam卡是谁掏钱投资的。
drop table IF EXISTS samcard_channel_info;
create table samcard_channel_info
(
  channel_id       numeric(4) not null,         -- 终端投资方机构的编号
  channel_name     VARCHAR(50) not null,      -- 终端投资方名称
  restrict_trade   numeric(1) default 0 not null,      -- 终端投资方交易限制标示，默认为0，不受限制。如果为1，那么就是限制交易。
  channelkey_index	numeric(3) -- 该渠道对应的密钥索引
)DEFAULT CHARSET=utf8;

alter table samcard_channel_info
  add constraint samcard_channel_info_PKEY primary key (channel_id);
  
  insert into samcard_channel_info(channel_id, channel_name, restrict_trade)values(1,'电信',0);
  commit;  

-- psam卡信息表
drop table IF EXISTS SAMCARD_INFO;
create table SAMCARD_INFO
(
  sam_card       VARCHAR(16) not null,   -- psam卡号
  channel_id     numeric(4) default 0 not null,      -- psam卡所属的渠道
  sam_model      VARCHAR(1) not null,    -- psam类型
  sam_status     VARCHAR(2) not null,    -- psam卡状态
  sam_signinflag VARCHAR(1),
  sam_zmk_index  numeric(1),
  sam_zmk        VARCHAR(32),
  sam_zmk1       VARCHAR(32),
  sam_zmk2       VARCHAR(32),
  sam_zmk3       VARCHAR(32),
  sam_pinkey_idx numeric(3),
  sam_pinkey     VARCHAR(32), 
  sam_mackey_idx numeric(3),
  sam_mackey     VARCHAR(32),
  sam_tdkey_idx  numeric(3),
  sam_tdkey      VARCHAR(32),
  pinkey_idx     numeric(3) default 1 not null,
  pinkey         VARCHAR(16) default 'C3EEA788F792E154' not null,
  mackey_idx     numeric(3),
  mackey         VARCHAR(16),
  pin_convflag   numeric(1) default 1 not null,
  startime       timestamp(6),
  endtime        timestamp(6) 
)DEFAULT CHARSET=utf8;

alter table SAMCARD_INFO
  add constraint SAMCARD_INFO_pk primary key (sam_card);

create index SAMCARD_INFO_index0 on SAMCARD_INFO (channel_id);

create index SAMCARD_INFO_index1 on SAMCARD_INFO (channel_id, sam_status);

create unique index SAMCARD_INFO_index2 on SAMCARD_INFO (sam_card, sam_status);

insert into SAMCARD_INFO (sam_card, channel_id, sam_model, sam_status, sam_signinflag, sam_zmk_index, sam_zmk, sam_zmk1, sam_zmk2, sam_zmk3, sam_pinkey_idx, sam_pinkey, sam_mackey_idx, sam_mackey, sam_tdkey_idx, sam_tdkey, pinkey_idx, pinkey, mackey_idx, mackey, pin_convflag)
values ('CADE000012345678', 1, '1', '00', '1', 0, null, null, null, null, 1, null, 2, null, 3, null, 1, 'C3EEA788F792E154', null, null, 1);
commit;

-- 终端商户限制表，表示限制该终端上某个商户的信用卡日限额限制
drop table IF EXISTS samcard_mertrade_restrict;
create table samcard_mertrade_restrict
(
  psam_card        VARCHAR(16) not null,  -- psam卡号
  mercode		   VARCHAR(15) not null,  -- 商户号
  c_amttotal       char(12) default '-',  -- 默认-无上限
  c_amtdate        VARCHAR(8), -- 表示某个交易日
  c_total_tranamt  char(12) default '000000000000' -- 信用卡当日交易总额
)DEFAULT CHARSET=utf8;

alter table samcard_mertrade_restrict
  add constraint samcardmertraderestrictpk primary key (psam_card, mercode);
  
  insert into samcard_mertrade_restrict(psam_card, mercode, c_amtdate)values('CADE000012345678','860010030210001','20120605');
  insert into samcard_mertrade_restrict(psam_card, mercode, c_amtdate)values('CADE000012345678','860010030210002','20120605');
  insert into samcard_mertrade_restrict(psam_card, mercode, c_amtdate)values('CADE000012345678','860010030210003','20120605');
  commit;  


-- psam卡状态表，对应的SAMCARD_INFO表的sam_status字段
drop table IF EXISTS samcard_status;
create table samcard_status
(
    samcard_status_code       VARCHAR(2) not null,
	samcard_status_desc		  VARCHAR(25) not null,
	xpep_ret_code       VARCHAR(2) not null
)DEFAULT CHARSET=utf8;

alter table samcard_status
  add constraint samcard_status_pk primary key (samcard_status_code);

insert into samcard_status values('00','正常','00');
insert into samcard_status values('01','终端审核中','FR');
insert into samcard_status values('02','终端撤机中','FS');
insert into samcard_status values('03','终端停机中','FT');
insert into samcard_status values('04','终端复机中','FU');
insert into samcard_status values('05','终端未使用','FV');
insert into samcard_status values('06','终端使用中','FW');
insert into samcard_status values('07','终端已停机','FX');

commit;

-- 终端限制表，对于该psam卡上发生的交易都需要做处理
drop table IF EXISTS SAMCARD_TRADE_RESTRICT;
create table SAMCARD_TRADE_RESTRICT
(
  psam_card      VARCHAR(16) not null,
  restrict_trade VARCHAR(27) default '-',
  restrict_card  char(1) default '-',
  passwd         char(1) default '-',
  amtmin         char(12) default '-', -- 单笔最小金额
  amtmax         char(12) default '-', -- 单笔最大金额
  c_amttotal       char(12) default '-',-- 信用卡日限额判断
  c_amtdate        VARCHAR(8),
  c_total_tranamt  char(12) default '000000000000',
  d_amttotal       char(12) default '-',-- 借记卡日限额判断
  d_amtdate        VARCHAR(8),
  d_total_tranamt  char(12) default '000000000000',
  fail_count     numeric(4) default -1,
  success_count  numeric(4) default -1
)DEFAULT CHARSET=utf8;

alter table SAMCARD_TRADE_RESTRICT
  add constraint SAMCARD_TRADE_RESTRICT_pk primary key (psam_card);
  
insert into SAMCARD_TRADE_RESTRICT(psam_card)values('CADE000012345678');

commit;

-- 终端投资方交易限制表
drop table IF EXISTS samcardchannel_to_msgchginfo;
create table samcardchannel_to_msgchginfo
(
  channel_id       numeric(4) not null,       -- 终端投资方机构的编号
  trancde          CHAR(3) not null,     -- 交易码
  restrict_trade   numeric(1)	default 0 not null     -- 交易限制标示，默认为0，不受限制。如果为1，那么就是限制交易。
)DEFAULT CHARSET=utf8;

alter table samcardchannel_to_msgchginfo
  add constraint samcardchannelmsgchginfo_PKEY primary key (channel_id, trancde);

alter table samcardchannel_to_msgchginfo
  add constraint SAMCARDCHANNELMSGCHGINFO_fk1 foreign key (CHANNEL_ID)
  references samcard_channel_info (CHANNEL_ID) on delete cascade;

alter table samcardchannel_to_msgchginfo
  add constraint SAMCARDCHANNELMSGCHGINFO_FK2 foreign key (TRANCDE)
  references msgchg_info (TRANCDE) on delete cascade;
  
  
insert into samcardchannel_to_msgchginfo(channel_id, trancde, restrict_trade)values(1,'101',0);
insert into samcardchannel_to_msgchginfo(channel_id, trancde, restrict_trade)values(1,'102',0);
insert into samcardchannel_to_msgchginfo(channel_id, trancde, restrict_trade)values(1,'103',0);
insert into samcardchannel_to_msgchginfo(channel_id, trancde, restrict_trade)values(1,'104',0);
insert into samcardchannel_to_msgchginfo(channel_id, trancde, restrict_trade)values(1,'105',0);
insert into samcardchannel_to_msgchginfo(channel_id, trancde, restrict_trade)values(1,'106',0);
insert into samcardchannel_to_msgchginfo(channel_id, trancde, restrict_trade)values(1,'107',0);
commit;

-- sms_jnls 表
drop table IF EXISTS sms_jnls;
create table sms_jnls
(
	trancde	VARCHAR(3) not null,
	outcard	VARCHAR(19),
	incard	VARCHAR(19),
	tradeamt VARCHAR(12),
	tradefee VARCHAR(12),
	tradedate VARCHAR(14) not null,
	traderesp VARCHAR(2),
	tradetrace	VARCHAR(6) not null,
	traderefer	VARCHAR(12),
	tradebillmsg	VARCHAR(40),
	userphone	VARCHAR(11) not null,
	smsresult	VARCHAR(10),
	allsmsdata	VARCHAR(150) not null,
	sms_datatime TIMESTAMP(6) not null
)DEFAULT CHARSET=utf8;

alter table sms_jnls
  add constraint sms_jnls_pk primary key (trancde, tradedate, userphone, tradetrace);

create index sms_jnls_index on sms_jnls(allsmsdata);

commit;

-- xpep_errtrade_jnls表
drop table IF EXISTS xpep_errtrade_jnls;
create table xpep_errtrade_jnls
(
  term_date    varchar(8) NOT NULL,
  term_time    varchar(6) NOT NULL,
  term_tel     varchar(25),
  term_psam    varchar(20) NOT NULL,
  term_e1code  varchar(16),
  term_error   varchar(64),
  term_trace   varchar(6),
  term_resp    varchar(3) NOT NULL,
  term_trancde varchar(4) NOT NULL
)DEFAULT CHARSET=utf8;

alter table xpep_errtrade_jnls add constraint xpep_errtrade_jnls_pk primary key (term_date, term_time, term_psam);


-- MySQL 用自增主键来实现

-- 前置交易流水号
DROP TABLE IF EXISTS xpep_jnls_trace_sequence;
CREATE TABLE xpep_jnls_trace_sequence (
    name              VARCHAR(50) NOT NULL,
    current_value INT NOT NULL,
    increment       INT NOT NULL DEFAULT 1,
    PRIMARY KEY (name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
INSERT INTO xpep_jnls_trace_sequence VALUES ('trace_seq',1,1);


--  前置系统应答码表，包括错误应答码和正常的应答码，和应答码的描述
drop table IF EXISTS xpep_retcode;
create table xpep_retcode
(
  xpep_trancde   CHAR(3) not null,  -- 交易码
  xpep_retcode   CHAR(2) not null,  -- 交易应答码
  xpep_content   VARCHAR(150) not null, -- 交易应答码描述
  xpep_content_ewp   VARCHAR(255) not null, -- 返回EWP的响应码描述信息，utf-8
  xpep_content_ewp_print   VARCHAR(500) not null, -- 返回EWP的打印数据，utf-8
  xpep_desc      VARCHAR(160)       -- 其它描述信息
)DEFAULT CHARSET=utf8;
alter table xpep_retcode
  add constraint xpep_retcode_pk primary key (xpep_trancde, xpep_retcode);

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
-- -- -- -- -- -- add by he.qingqi  8/31   -- -- -- -
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','00','承兑或交易成功','\u627f\u5151\u6216\u4ea4\u6613\u6210\u529f','\u627f\u5151\u6216\u4ea4\u6613\u6210\u529f');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','01','查发卡行','\u67e5\u53d1\u5361\u884c','\u67e5\u53d1\u5361\u884c');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','02','查发卡行的特殊条件','\u53ef\u7535\u8bdd\u5411\u53d1\u5361\u884c\u67e5\u8be2','\u53ef\u7535\u8bdd\u5411\u53d1\u5361\u884c\u67e5\u8be2');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','03','无效商户','\u5546\u6237\u9700\u8981\u5728\u94f6\u884c\u6216\u4e2d\u5fc3\u767b\u8bb0','\u5546\u6237\u9700\u8981\u5728\u94f6\u884c\u6216\u4e2d\u5fc3\u767b\u8bb0');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','04','没收卡','\u64cd\u4f5c\u5458\u6ca1\u6536\u5361','\u64cd\u4f5c\u5458\u6ca1\u6536\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','05','不予承兑','\u53d1\u5361\u4e0d\u4e88\u627f\u5151','\u53d1\u5361\u4e0d\u4e88\u627f\u5151');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','06','出错','\u53d1\u5361\u884c\u6545\u969c','\u53d1\u5361\u884c\u6545\u969c');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','07','特殊条件下没收卡','\u7279\u6b8a\u6761\u4ef6\u4e0b\u6ca1\u6536\u5361','\u7279\u6b8a\u6761\u4ef6\u4e0b\u6ca1\u6536\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','08','请求正在处理中','\u91cd\u65b0\u63d0\u4ea4\u4ea4\u6613\u8bf7\u6c42','\u91cd\u65b0\u63d0\u4ea4\u4ea4\u6613\u8bf7\u6c42');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','09','无效交易','\u53d1\u5361\u884c\u4e0d\u652f\u6301\u7684\u4ea4\u6613','\u53d1\u5361\u884c\u4e0d\u652f\u6301\u7684\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','12','无效交易','\u65e0\u6548\u4ea4\u6613','\u65e0\u6548\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','13','无效金额','\u91d1\u989d\u4e3a0\u6216\u592a\u5927','\u91d1\u989d\u4e3a0\u6216\u592a\u5927');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','14','无效卡号','\u5361\u79cd\u672a\u5728\u4e2d\u5fc3\u767b\u8bb0\u6216\u8bfb\u5361\u53f7\u6709\u8bef','\u5361\u79cd\u672a\u5728\u4e2d\u5fc3\u767b\u8bb0\u6216\u8bfb\u5361\u53f7\u6709\u8bef');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','15','无此发卡行','\u6b64\u53d1\u5361\u884c\u672a\u4e0e\u4e2d\u5fc3\u5f00\u901a\u4e1a\u52a1','\u6b64\u53d1\u5361\u884c\u672a\u4e0e\u4e2d\u5fc3\u5f00\u901a\u4e1a\u52a1');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','16','批准更新第三磁道','\u6279\u51c6\u66f4\u65b0\u7b2c\u4e09\u78c1\u9053','\u6279\u51c6\u66f4\u65b0\u7b2c\u4e09\u78c1\u9053');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','19','重新送入交易','\u5237\u5361\u8bfb\u53d6\u6570\u636e\u6709\u8bef\uff0c\u53ef\u91cd\u65b0\u5237\u5361','\u5237\u5361\u8bfb\u53d6\u6570\u636e\u6709\u8bef\uff0c\u53ef\u91cd\u65b0\u5237\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','20','无效应答','\u65e0\u6548\u5e94\u7b54','\u65e0\u6548\u5e94\u7b54');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','21','不做任何处理','\u4e0d\u505a\u4efb\u4f55\u5904\u7406','\u4e0d\u505a\u4efb\u4f55\u5904\u7406');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','22','怀疑操作有误','POS\u72b6\u6001\u4e0e\u4e2d\u5fc3\u4e0d\u7b26\uff0c\u53ef\u91cd\u65b0\u7b7e\u5230','POS\u72b6\u6001\u4e0e\u4e2d\u5fc3\u4e0d\u7b26\uff0c\u53ef\u91cd\u65b0\u7b7e\u5230');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','23','不可接受的交易费','\u4e0d\u53ef\u63a5\u53d7\u7684\u4ea4\u6613\u8d39','\u4e0d\u53ef\u63a5\u53d7\u7684\u4ea4\u6613\u8d39');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','25','未能找到文件上记录','\u53d1\u5361\u884c\u672a\u80fd\u627e\u5230\u6709\u5173\u8bb0\u5f55','\u53d1\u5361\u884c\u672a\u80fd\u627e\u5230\u6709\u5173\u8bb0\u5f55');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','30','格式错误','\u683c\u5f0f\u9519\u8bef','\u683c\u5f0f\u9519\u8bef');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','31','银联不支持的银行','\u6b64\u53d1\u5361\u65b9\u672a\u4e0e\u4e2d\u5fc3\u5f00\u901a\u4e1a\u52a1','\u6b64\u53d1\u5361\u65b9\u672a\u4e0e\u4e2d\u5fc3\u5f00\u901a\u4e1a\u52a1');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','33','过期的卡','\u8fc7\u671f\u7684\u5361\uff0c\u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536','\u8fc7\u671f\u7684\u5361\uff0c\u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','34','有作弊嫌疑','\u6709\u4f5c\u5f0a\u5acc\u7591\u7684\u5361\uff0c\u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536','\u6709\u4f5c\u5f0a\u5acc\u7591\u7684\u5361\uff0c\u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','35','受卡方与安全保密部门联系','\u6709\u4f5c\u5f0a\u5acc\u7591\u7684\u5361\uff0c\u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536','\u6709\u4f5c\u5f0a\u5acc\u7591\u7684\u5361\uff0c\u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','36','受限制的卡','\u8be5\u5361\u79cd\u4e0d\u53d7\u7406','\u8be5\u5361\u79cd\u4e0d\u53d7\u7406');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','37','受卡方呼受理方安全保密部门(没收卡)','\u6709\u4f5c\u5f0a\u5acc\u7591\u7684\u5361\uff0c\u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536','\u6709\u4f5c\u5f0a\u5acc\u7591\u7684\u5361\uff0c\u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','38','超过允许的PIN试输入','\u5bc6\u7801\u9519\u6b21\u6570\u8d85\u9650\uff0c\u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536','\u5bc6\u7801\u9519\u6b21\u6570\u8d85\u9650\uff0c\u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','39','无此信用卡账户','\u53ef\u80fd\u5237\u5361\u64cd\u4f5c\u6709\u8bef','\u53ef\u80fd\u5237\u5361\u64cd\u4f5c\u6709\u8bef');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','40','请求的功能尚不支持','\u53d1\u5361\u884c\u4e0d\u652f\u6301\u7684\u4ea4\u6613\u7c7b\u578b','\u53d1\u5361\u884c\u4e0d\u652f\u6301\u7684\u4ea4\u6613\u7c7b\u578b');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','41','丢失卡','\u6302\u5931\u7684\u5361\uff0c \u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536','\u6302\u5931\u7684\u5361\uff0c \u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','42','无此账户','\u53d1\u5361\u884c\u627e\u4e0d\u5230\u6b64\u8d26\u6237','\u53d1\u5361\u884c\u627e\u4e0d\u5230\u6b64\u8d26\u6237');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','43','被窃卡','\u88ab\u7a83\u5361\uff0c \u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536','\u88ab\u7a83\u5361\uff0c \u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','44','无此投资账户','\u53ef\u80fd\u5237\u5361\u64cd\u4f5c\u6709\u8bef','\u53ef\u80fd\u5237\u5361\u64cd\u4f5c\u6709\u8bef');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','51','无足够的存款','\u8d26\u6237\u5185\u4f59\u989d\u4e0d\u8db3','\u8d26\u6237\u5185\u4f59\u989d\u4e0d\u8db3');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','52','无此支票账户','\u65e0\u6b64\u652f\u7968\u8d26\u6237','\u65e0\u6b64\u652f\u7968\u8d26\u6237');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','53','无此储蓄卡账户','\u65e0\u6b64\u50a8\u84c4\u5361\u8d26\u6237','\u65e0\u6b64\u50a8\u84c4\u5361\u8d26\u6237');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','54','过期的卡','\u8fc7\u671f\u7684\u5361','\u8fc7\u671f\u7684\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','55','不正确的PIN','\u5bc6\u7801\u8f93\u9519','\u5bc6\u7801\u8f93\u9519');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','56','无此卡记录','\u53d1\u5361\u884c\u627e\u4e0d\u5230\u6b64\u8d26\u6237','\u53d1\u5361\u884c\u627e\u4e0d\u5230\u6b64\u8d26\u6237');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','57','不允许持卡人进行的交易','\u4e0d\u5141\u8bb8\u6301\u5361\u4eba\u8fdb\u884c\u7684\u4ea4\u6613','\u4e0d\u5141\u8bb8\u6301\u5361\u4eba\u8fdb\u884c\u7684\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','58','不允许终端进行的交易','\u8be5\u5546\u6237\u4e0d\u5141\u8bb8\u8fdb\u884c\u7684\u4ea4\u6613','\u8be5\u5546\u6237\u4e0d\u5141\u8bb8\u8fdb\u884c\u7684\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','59','有作弊嫌疑','\u6709\u4f5c\u5f0a\u5acc\u7591','\u6709\u4f5c\u5f0a\u5acc\u7591');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','60','受卡方与安全保密部门联系','\u53d7\u5361\u65b9\u4e0e\u5b89\u5168\u4fdd\u5bc6\u90e8\u95e8\u8054\u7cfb','\u53d7\u5361\u65b9\u4e0e\u5b89\u5168\u4fdd\u5bc6\u90e8\u95e8\u8054\u7cfb');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','61','超出取款金额限制','\u4e00\u6b21\u4ea4\u6613\u7684\u91d1\u989d\u592a\u5927','\u4e00\u6b21\u4ea4\u6613\u7684\u91d1\u989d\u592a\u5927');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','62','受限制的卡','\u53d7\u9650\u5236\u7684\u5361\u6216\u78c1\u6761\u4fe1\u606f\u6709\u8bef','\u53d7\u9650\u5236\u7684\u5361\u6216\u78c1\u6761\u4fe1\u606f\u6709\u8bef');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','63','违反安全保密规定','\u8fdd\u53cd\u5b89\u5168\u4fdd\u5bc6\u89c4\u5b9a','\u8fdd\u53cd\u5b89\u5168\u4fdd\u5bc6\u89c4\u5b9a');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','64','原始金额不正确','\u539f\u59cb\u91d1\u989d\u4e0d\u6b63\u786e','\u539f\u59cb\u91d1\u989d\u4e0d\u6b63\u786e');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','65','超出取款次数限制','\u8d85\u51fa\u53d6\u6b3e\u6b21\u6570\u9650\u5236','\u8d85\u51fa\u53d6\u6b3e\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','66','受卡方呼受理方安全保密部门','\u53d7\u5361\u65b9\u547c\u53d7\u7406\u65b9\u5b89\u5168\u4fdd\u5bc6\u90e8\u95e8','\u53d7\u5361\u65b9\u547c\u53d7\u7406\u65b9\u5b89\u5168\u4fdd\u5bc6\u90e8\u95e8');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','67','捕捉（没收卡）','\u6355\u6349\uff08\u6ca1\u6536\u5361\uff09','\u6355\u6349\uff08\u6ca1\u6536\u5361\uff09');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','68','收到的回答太迟','\u53d1\u5361\u884c\u89c4\u5b9a\u65f6\u95f4\u5185\u6ca1\u6709\u56de\u7b54','\u53d1\u5361\u884c\u89c4\u5b9a\u65f6\u95f4\u5185\u6ca1\u6709\u56de\u7b54');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','75','允许的输入PIN次数超限','\u5141\u8bb8\u7684\u8f93\u5165PIN\u6b21\u6570\u8d85\u9650','\u5141\u8bb8\u7684\u8f93\u5165PIN\u6b21\u6570\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','77','需要向网络中心签到','POS\u6279\u6b21\u4e0e\u7f51\u7edc\u4e2d\u5fc3\u4e0d\u4e00\u81f4','POS\u6279\u6b21\u4e0e\u7f51\u7edc\u4e2d\u5fc3\u4e0d\u4e00\u81f4');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','79','脱机交易对账不平','POS\u7ec8\u7aef\u4e0a\u4f20\u7684\u8131\u673a\u6570\u636e\u5bf9\u8d26\u4e0d\u5e73','POS\u7ec8\u7aef\u4e0a\u4f20\u7684\u8131\u673a\u6570\u636e\u5bf9\u8d26\u4e0d\u5e73');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','90','日期切换正在处理','\u65e5\u671f\u5207\u6362\u6b63\u5728\u5904\u7406','\u65e5\u671f\u5207\u6362\u6b63\u5728\u5904\u7406');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','91','发卡行或银联不能操作','\u7535\u8bdd\u67e5\u8be2\u53d1\u5361\u65b9\u6216\u94f6\u8054\uff0c\u53ef\u91cd\u4f5c','\u7535\u8bdd\u67e5\u8be2\u53d1\u5361\u65b9\u6216\u94f6\u8054\uff0c\u53ef\u91cd\u4f5c');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','92','金融机构或中间网络设施找不到或无法达到','\u7535\u8bdd\u67e5\u8be2\u53d1\u5361\u65b9\u6216\u7f51\u7edc\u4e2d\u5fc3\uff0c\u53ef\u91cd\u4f5c','\u7535\u8bdd\u67e5\u8be2\u53d1\u5361\u65b9\u6216\u7f51\u7edc\u4e2d\u5fc3\uff0c\u53ef\u91cd\u4f5c');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','93','交易违法、不能完成','\u4ea4\u6613\u8fdd\u6cd5\u3001\u4e0d\u80fd\u5b8c\u6210','\u4ea4\u6613\u8fdd\u6cd5\u3001\u4e0d\u80fd\u5b8c\u6210');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','94','重复交易','\u67e5\u8be2\u7f51\u7edc\u4e2d\u5fc3\uff0c\u53ef\u91cd\u65b0\u7b7e\u5230\u4f5c\u4ea4\u6613','\u67e5\u8be2\u7f51\u7edc\u4e2d\u5fc3\uff0c\u53ef\u91cd\u65b0\u7b7e\u5230\u4f5c\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','95','调节控制错','\u8c03\u8282\u63a7\u5236\u9519','\u8c03\u8282\u63a7\u5236\u9519');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','96','系统异常、失效','\u53d1\u5361\u65b9\u6216\u7f51\u7edc\u4e2d\u5fc3\u51fa\u73b0\u6545\u969c','\u53d1\u5361\u65b9\u6216\u7f51\u7edc\u4e2d\u5fc3\u51fa\u73b0\u6545\u969c');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','97','POS终端号找不到','\u7ec8\u7aef\u672a\u5728\u4e2d\u5fc3\u6216\u94f6\u884c\u767b\u8bb0','\u7ec8\u7aef\u672a\u5728\u4e2d\u5fc3\u6216\u94f6\u884c\u767b\u8bb0');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','98','银联收不到发卡行应答','\u94f6\u8054\u6536\u4e0d\u5230\u53d1\u5361\u884c\u5e94\u7b54','\u94f6\u8054\u6536\u4e0d\u5230\u53d1\u5361\u884c\u5e94\u7b54');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','99','PIN格式错','PIN\u683c\u5f0f\u9519\uff0c\u53ef\u91cd\u65b0\u7b7e\u5230\u4f5c\u4ea4\u6613','PIN\u683c\u5f0f\u9519\uff0c\u53ef\u91cd\u65b0\u7b7e\u5230\u4f5c\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','F0','消费撤销或是退货时找不到原笔交易','\u672a\u80fd\u67e5\u8be2\u5230\u539f\u7b14\u4ea4\u6613','\u672a\u80fd\u67e5\u8be2\u5230\u539f\u7b14\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','F1','解析接入系统发过来的报文数据失败','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','F2','内部商户交易卡种限制，该商户不支持此银行卡','\u8be5\u5546\u6237\u4e0d\u652f\u6301\u6b64\u94f6\u884c\u5361','\u8be5\u5546\u6237\u4e0d\u652f\u6301\u6b64\u94f6\u884c\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','F3','解密磁道错误','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','F4','查询类：想要查询的交易不存在','\u67e5\u8be2\u7684\u4ea4\u6613\u4e0d\u5b58\u5728','\u67e5\u8be2\u7684\u4ea4\u6613\u4e0d\u5b58\u5728');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','F5','组iso8583包失败','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','F6','商户交易金额限制,无效的交易金额','\u4ea4\u6613\u91d1\u989d\u53d7\u9650','\u4ea4\u6613\u91d1\u989d\u53d7\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','F7','转加密密码错误','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','F8','取前置系统的交易流水错误','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','F9','发送报文给核心系统失败','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FA','该交易不允许银行卡无密支付','\u4ea4\u6613\u53d7\u9650','\u4ea4\u6613\u53d7\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FB','查询msgchg_info失败,交易未开通','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FC','查询samcard_info失败,psam卡未找到','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FD','该渠道该交易不存在','\u4ea4\u6613\u4e0d\u5b58\u5728','\u4ea4\u6613\u4e0d\u5b58\u5728');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FE','该渠道限制交易','\u4ea4\u6613\u53d7\u9650','\u4ea4\u6613\u53d7\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FF','该渠道该交易未开通','\u4ea4\u6613\u672a\u5f00\u901a','\u4ea4\u6613\u672a\u5f00\u901a');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FG','该终端属于黑名单，不允许交易','\u4ea4\u6613\u53d7\u9650','\u4ea4\u6613\u53d7\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FH','终端卡种限制，未知当前交易卡种','\u4ea4\u6613\u53d7\u9650','\u4ea4\u6613\u53d7\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FI','终端卡种限制,受限制的卡','\u4ea4\u6613\u53d7\u9650','\u4ea4\u6613\u53d7\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FJ','终端限制交易金额,不在最大最小金额的范围','\u4ea4\u6613\u53d7\u9650','\u4ea4\u6613\u53d7\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FK','终端卡种无法判断，又有交易日限额的控制','\u4ea4\u6613\u53d7\u9650','\u4ea4\u6613\u53d7\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FL','该终端当前的交易总额，超过总交易额的限制','\u4ea4\u6613\u53d7\u9650','\u4ea4\u6613\u53d7\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FM','操作数据库表商户黑名单失败','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FN','商户黑名单，该商户不允许该交易','\u4ea4\u6613\u53d7\u9650','\u4ea4\u6613\u53d7\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FO','保存pep_jnls数据失败','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FP','保存EWP_INFO失败','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FQ','终端正常，可以使用','\u7ec8\u7aef\u6b63\u5e38\uff0c\u53ef\u4ee5\u4f7f\u7528','\u7ec8\u7aef\u6b63\u5e38\uff0c\u53ef\u4ee5\u4f7f\u7528');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FR','终端审核中','\u7ec8\u7aef\u5ba1\u6838\u4e2d','\u7ec8\u7aef\u5ba1\u6838\u4e2d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FS','终端撤机中','\u7ec8\u7aef\u64a4\u673a\u4e2d','\u7ec8\u7aef\u64a4\u673a\u4e2d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FT','终端停机中','\u7aef\u505c\u673a\u4e2d','\u7aef\u505c\u673a\u4e2d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FU','终端复机中','\u7ec8\u7aef\u590d\u673a\u4e2d','\u7ec8\u7aef\u590d\u673a\u4e2d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FV','终端未使用','\u7ec8\u7aef\u672a\u4f7f\u7528','\u7ec8\u7aef\u672a\u4f7f\u7528');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FW','终端使用中','\u7ec8\u7aef\u4f7f\u7528\u4e2d','\u7ec8\u7aef\u4f7f\u7528\u4e2d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FX','终端已停机','\u7ec8\u7aef\u5df2\u505c\u673a','\u7ec8\u7aef\u5df2\u505c\u673a');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FY','查询psam当前的状态信息失败','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FZ','数据库操作错误','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','H0','订单支付状态未知，5分钟之内不允许再次支付','\u4ea4\u6613\u53d7\u9650','\u4ea4\u6613\u53d7\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','E0','系统交易错误','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','EN','加密机计算错误，请检查加密机连接','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Sl','商户密钥错误，需要先签到','\u5546\u6237\u672a\u7b7e\u5230','\u5546\u6237\u672a\u7b7e\u5230');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','N1','超时','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','N2','交易失败','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','A0','POS 终端对收到 POS 中心的批准应答消息,验证 MAC 出错','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','A1','异常交易','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','A2','冲正超时','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','A3','冲正成功','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','A4','冲正异常错误','\u51b2\u6b63\u5f02\u5e38\u9519\u8bef','\u51b2\u6b63\u5f02\u5e38\u9519\u8bef');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','C1','无效的商户号','\u65e0\u6548\u5546\u6237','\u65e0\u6548\u5546\u6237');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','C2','无效的终端号','\u65e0\u6548\u7ec8\u7aef','\u65e0\u6548\u7ec8\u7aef');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','C3','商户扣款路由配置不存在','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','C4','该交易金额发生过退货或者撤销，不能再次撤销','\u8be5\u4ea4\u6613\u91d1\u989d\u5df2\u9000\u8d27\u6216\u8005\u64a4\u9500','\u8be5\u4ea4\u6613\u91d1\u989d\u5df2\u9000\u8d27\u6216\u8005\u64a4\u9500');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','C5','未能找到原笔交易','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','C6','退货交易金额超限','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','C7','退货撤销银联返回结果异常','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010');

-- add by lp 8/31 16:50
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','H1','由于系统或者其他原因，交易暂时关闭','\u7531\u4e8e\u7cfb\u7edf\u6216\u8005\u5176\u4ed6\u539f\u56e0\uff0c\u4ea4\u6613\u6682\u65f6\u5173\u95ed','\u7531\u4e8e\u7cfb\u7edf\u6216\u8005\u5176\u4ed6\u539f\u56e0\uff0c\u4ea4\u6613\u6682\u65f6\u5173\u95ed');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','H2','收款交易终端未开通','\u6536\u6b3e\u4ea4\u6613\u7ec8\u7aef\u672a\u5f00\u901a','\u6536\u6b3e\u4ea4\u6613\u7ec8\u7aef\u672a\u5f00\u901a');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','H3','历史交易查询没有记录','\u5386\u53f2\u4ea4\u6613\u67e5\u8be2\u6ca1\u6709\u8bb0\u5f55','\u5386\u53f2\u4ea4\u6613\u67e5\u8be2\u6ca1\u6709\u8bb0\u5f55');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','H4','取POS终端上送的41域错误','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','H5','取POS终端上送的42域错误','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','H6','取POS终端上送的60域错误','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','H7','设置冲正字段失败','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','H8','消费撤销是否需要磁密信息和后台不匹配','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','H9','终端上送的报文格式不对','\u7ec8\u7aef\u4e0a\u9001\u7684\u62a5\u6587\u683c\u5f0f\u4e0d\u5bf9','\u7ec8\u7aef\u4e0a\u9001\u7684\u62a5\u6587\u683c\u5f0f\u4e0d\u5bf9');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','HA','终端未入库','\u7ec8\u7aef\u672a\u5165\u5e93','\u7ec8\u7aef\u672a\u5165\u5e93');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','HB','签到失败','\u7b7e\u5230\u5931\u8d25','\u7b7e\u5230\u5931\u8d25');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','HC','不是刷卡交易暂时不受理','\u4e0d\u662f\u5237\u5361\u4ea4\u6613\u6682\u65f6\u4e0d\u53d7\u7406','\u4e0d\u662f\u5237\u5361\u4ea4\u6613\u6682\u65f6\u4e0d\u53d7\u7406');


insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','T1','参数下载时，批次号不一致并且没有做过批结算','\u53c2\u6570\u4e0b\u8f7d\u5931\u8d25\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u53c2\u6570\u4e0b\u8f7d\u5931\u8d25\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','T3','参数下载时，批次号一致，但是没有做过批结算','\u53c2\u6570\u4e0b\u8f7d\u5931\u8d25\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u53c2\u6570\u4e0b\u8f7d\u5931\u8d25\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','T4','批结算时，上送的批次号不一致','\u7ed3\u7b97\u5931\u8d25\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u7ed3\u7b97\u5931\u8d25\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');


-- 新增加核心系统风控的交易应答码 
-- he.qingqi
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','V1','限制卡Bin进行交易','\u9650\u5236\u5361Bin\u8fdb\u884c\u4ea4\u6613','\u9650\u5236\u5361Bin\u8fdb\u884c\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','V2','不能低于卡Bin限制的最低金额','\u4e0d\u80fd\u4f4e\u4e8e\u5361Bin\u9650\u5236\u7684\u6700\u4f4e\u91d1\u989d','\u4e0d\u80fd\u4f4e\u4e8e\u5361Bin\u9650\u5236\u7684\u6700\u4f4e\u91d1\u989d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','V3','不能低于卡Bin限制的最高金额','\u4e0d\u80fd\u4f4e\u4e8e\u5361Bin\u9650\u5236\u7684\u6700\u9ad8\u91d1\u989d','\u4e0d\u80fd\u4f4e\u4e8e\u5361Bin\u9650\u5236\u7684\u6700\u9ad8\u91d1\u989d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','V4','不能低于卡Bin限制的日交易最低金额','\u4e0d\u80fd\u4f4e\u4e8e\u5361Bin\u9650\u5236\u7684\u65e5\u4ea4\u6613\u6700\u4f4e\u91d1\u989d','\u4e0d\u80fd\u4f4e\u4e8e\u5361Bin\u9650\u5236\u7684\u65e5\u4ea4\u6613\u6700\u4f4e\u91d1\u989d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','V5','不能低于卡Bin限制的日交易最高金额','\u4e0d\u80fd\u4f4e\u4e8e\u5361Bin\u9650\u5236\u7684\u65e5\u4ea4\u6613\u6700\u9ad8\u91d1\u989d','\u4e0d\u80fd\u4f4e\u4e8e\u5361Bin\u9650\u5236\u7684\u65e5\u4ea4\u6613\u6700\u9ad8\u91d1\u989d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','W1','该银行卡黑名单被限制不能交易','\u8be5\u94f6\u884c\u5361\u9ed1\u540d\u5355\u88ab\u9650\u5236\u4e0d\u80fd\u4ea4\u6613','\u8be5\u94f6\u884c\u5361\u9ed1\u540d\u5355\u88ab\u9650\u5236\u4e0d\u80fd\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','W2','该银行卡黑名单单笔交易金额超限','\u8be5\u94f6\u884c\u5361\u9ed1\u540d\u5355\u5355\u7b14\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u94f6\u884c\u5361\u9ed1\u540d\u5355\u5355\u7b14\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','W3','该银行卡黑名单日交易金额超限','\u8be5\u94f6\u884c\u5361\u9ed1\u540d\u5355\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u94f6\u884c\u5361\u9ed1\u540d\u5355\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','W4','限制该银行卡进行交易','\u9650\u5236\u8be5\u94f6\u884c\u5361\u8fdb\u884c\u4ea4\u6613','\u9650\u5236\u8be5\u94f6\u884c\u5361\u8fdb\u884c\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','W5','不能低于该银行卡最低金额','\u4e0d\u80fd\u4f4e\u4e8e\u8be5\u94f6\u884c\u5361\u6700\u4f4e\u91d1\u989d','\u4e0d\u80fd\u4f4e\u4e8e\u8be5\u94f6\u884c\u5361\u6700\u4f4e\u91d1\u989d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','W6','不能低于该银行卡最高金额','\u4e0d\u80fd\u4f4e\u4e8e\u8be5\u94f6\u884c\u5361\u6700\u9ad8\u91d1\u989d','\u4e0d\u80fd\u4f4e\u4e8e\u8be5\u94f6\u884c\u5361\u6700\u9ad8\u91d1\u989d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','W7','不能低于该银行卡日交易最低金额','\u4e0d\u80fd\u4f4e\u4e8e\u8be5\u94f6\u884c\u5361\u65e5\u4ea4\u6613\u6700\u4f4e\u91d1\u989d','\u4e0d\u80fd\u4f4e\u4e8e\u8be5\u94f6\u884c\u5361\u65e5\u4ea4\u6613\u6700\u4f4e\u91d1\u989d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','W8','不能低于该银行卡日交易最高金额','\u4e0d\u80fd\u4f4e\u4e8e\u8be5\u94f6\u884c\u5361\u65e5\u4ea4\u6613\u6700\u9ad8\u91d1\u989d','\u4e0d\u80fd\u4f4e\u4e8e\u8be5\u94f6\u884c\u5361\u65e5\u4ea4\u6613\u6700\u9ad8\u91d1\u989d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Y1','该终端黑名单不能交易','\u8be5\u7ec8\u7aef\u9ed1\u540d\u5355\u4e0d\u80fd\u4ea4\u6613','\u8be5\u7ec8\u7aef\u9ed1\u540d\u5355\u4e0d\u80fd\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Y2','该终端黑名单不能使用信用卡交易','\u8be5\u7ec8\u7aef\u9ed1\u540d\u5355\u4e0d\u80fd\u4f7f\u7528\u4fe1\u7528\u5361\u4ea4\u6613','\u8be5\u7ec8\u7aef\u9ed1\u540d\u5355\u4e0d\u80fd\u4f7f\u7528\u4fe1\u7528\u5361\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Y3','该终端黑名单下单笔交易金额超限','\u8be5\u7ec8\u7aef\u9ed1\u540d\u5355\u4e0b\u5355\u7b14\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u7ec8\u7aef\u9ed1\u540d\u5355\u4e0b\u5355\u7b14\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Y4','该终端黑名单下日交易金额超限','\u8be5\u7ec8\u7aef\u9ed1\u540d\u5355\u4e0b\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u7ec8\u7aef\u9ed1\u540d\u5355\u4e0b\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Y5','该终端不允许进行交易','\u8be5\u7ec8\u7aef\u4e0d\u5141\u8bb8\u8fdb\u884c\u4ea4\u6613','\u8be5\u7ec8\u7aef\u4e0d\u5141\u8bb8\u8fdb\u884c\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Y6','该终端下信用卡不能使用','\u8be5\u7ec8\u7aef\u4e0b\u4fe1\u7528\u5361\u4e0d\u80fd\u4f7f\u7528','\u8be5\u7ec8\u7aef\u4e0b\u4fe1\u7528\u5361\u4e0d\u80fd\u4f7f\u7528');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Y7','该终端下借记卡不能使用','\u8be5\u7ec8\u7aef\u4e0b\u501f\u8bb0\u5361\u4e0d\u80fd\u4f7f\u7528','\u8be5\u7ec8\u7aef\u4e0b\u501f\u8bb0\u5361\u4e0d\u80fd\u4f7f\u7528');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Y8','该终端下信用卡单笔金额超限','\u8be5\u7ec8\u7aef\u4e0b\u4fe1\u7528\u5361\u5355\u7b14\u91d1\u989d\u8d85\u9650','\u8be5\u7ec8\u7aef\u4e0b\u4fe1\u7528\u5361\u5355\u7b14\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Y9','该终端下借记卡单笔金额超限','\u8be5\u7ec8\u7aef\u4e0b\u501f\u8bb0\u5361\u5355\u7b14\u91d1\u989d\u8d85\u9650','\u8be5\u7ec8\u7aef\u4e0b\u501f\u8bb0\u5361\u5355\u7b14\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','YA','该终端下信用卡日交易金额超限','\u8be5\u7ec8\u7aef\u4e0b\u4fe1\u7528\u5361\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u7ec8\u7aef\u4e0b\u4fe1\u7528\u5361\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','YB','该终端下借记卡日交易金额超限','\u8be5\u7ec8\u7aef\u4e0b\u501f\u8bb0\u5361\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u7ec8\u7aef\u4e0b\u501f\u8bb0\u5361\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','X1','该商户不能撤销','\u8be5\u5546\u6237\u4e0d\u80fd\u64a4\u9500','\u8be5\u5546\u6237\u4e0d\u80fd\u64a4\u9500');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','X2','该商户不能退货','\u8be5\u5546\u6237\u4e0d\u80fd\u9000\u8d27','\u8be5\u5546\u6237\u4e0d\u80fd\u9000\u8d27');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','X3','该商户黑名单不能交易','\u8be5\u5546\u6237\u9ed1\u540d\u5355\u4e0d\u80fd\u4ea4\u6613','\u8be5\u5546\u6237\u9ed1\u540d\u5355\u4e0d\u80fd\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','X4','该商户黑名单不能使用信用卡交易','\u8be5\u5546\u6237\u9ed1\u540d\u5355\u4e0d\u80fd\u4f7f\u7528\u4fe1\u7528\u5361\u4ea4\u6613','\u8be5\u5546\u6237\u9ed1\u540d\u5355\u4e0d\u80fd\u4f7f\u7528\u4fe1\u7528\u5361\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','X5','该商户黑名单下单笔交易金额超限','\u8be5\u5546\u6237\u9ed1\u540d\u5355\u4e0b\u5355\u7b14\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u5546\u6237\u9ed1\u540d\u5355\u4e0b\u5355\u7b14\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','X6','该商户黑名单下日交易金额超限','\u8be5\u5546\u6237\u9ed1\u540d\u5355\u4e0b\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u5546\u6237\u9ed1\u540d\u5355\u4e0b\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','X7','该商户不能进行交易','\u8be5\u5546\u6237\u4e0d\u80fd\u8fdb\u884c\u4ea4\u6613','\u8be5\u5546\u6237\u4e0d\u80fd\u8fdb\u884c\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','X8','该商户不允许信用卡交易','\u8be5\u5546\u6237\u4e0d\u5141\u8bb8\u4fe1\u7528\u5361\u4ea4\u6613','\u8be5\u5546\u6237\u4e0d\u5141\u8bb8\u4fe1\u7528\u5361\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','X9','该商户不允许借记卡交易','\u8be5\u5546\u6237\u4e0d\u5141\u8bb8\u501f\u8bb0\u5361\u4ea4\u6613','\u8be5\u5546\u6237\u4e0d\u5141\u8bb8\u501f\u8bb0\u5361\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XA','该商户下信用卡单笔交易金额超限','\u8be5\u5546\u6237\u4e0b\u4fe1\u7528\u5361\u5355\u7b14\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u5546\u6237\u4e0b\u4fe1\u7528\u5361\u5355\u7b14\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XB','该商户下借记卡单笔交易金额超限','\u8be5\u5546\u6237\u4e0b\u501f\u8bb0\u5361\u5355\u7b14\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u5546\u6237\u4e0b\u501f\u8bb0\u5361\u5355\u7b14\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XC','该商户下信用卡日交易金额超限','\u8be5\u5546\u6237\u4e0b\u4fe1\u7528\u5361\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u5546\u6237\u4e0b\u4fe1\u7528\u5361\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XD','该商户下借记卡日交易金额超限','\u8be5\u5546\u6237\u4e0b\u501f\u8bb0\u5361\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u5546\u6237\u4e0b\u501f\u8bb0\u5361\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Z1','该渠道下不能进行交易','\u8be5\u6e20\u9053\u4e0b\u4e0d\u80fd\u8fdb\u884c\u4ea4\u6613','\u8be5\u6e20\u9053\u4e0b\u4e0d\u80fd\u8fdb\u884c\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Z2','该渠道下不能使用信用卡','\u8be5\u6e20\u9053\u4e0b\u4e0d\u80fd\u4f7f\u7528\u4fe1\u7528\u5361','\u8be5\u6e20\u9053\u4e0b\u4e0d\u80fd\u4f7f\u7528\u4fe1\u7528\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Z3','该渠道下不能使用借记卡','\u8be5\u6e20\u9053\u4e0b\u4e0d\u80fd\u4f7f\u7528\u501f\u8bb0\u5361','\u8be5\u6e20\u9053\u4e0b\u4e0d\u80fd\u4f7f\u7528\u501f\u8bb0\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Z4','该渠道下信用卡最低交易金额超限','\u8be5\u6e20\u9053\u4e0b\u4fe1\u7528\u5361\u6700\u4f4e\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u6e20\u9053\u4e0b\u4fe1\u7528\u5361\u6700\u4f4e\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Z5','该渠道下借记卡最低交易金额超限','\u8be5\u6e20\u9053\u4e0b\u501f\u8bb0\u5361\u6700\u4f4e\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u6e20\u9053\u4e0b\u501f\u8bb0\u5361\u6700\u4f4e\u4ea4\u6613\u91d1\u989d\u8d85\u9650');

-- add by lp 
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','A6','转账汇款代付超时,不确定是否汇款成功','\u4ea4\u6613\u8d85\u65f6','\u4ea4\u6613\u8d85\u65f6');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','V6','该商户下 不能高于卡Bin限制的最高金额','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','V7','该终端下 限制卡Bin进行交易','\u53d7\u9650\u5236\u7684\u5361','\u53d7\u9650\u5236\u7684\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','V8','该终端下 不能低于卡Bin限制的最低金额','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','V9','该终端下 不能高于卡Bin限制的最高金额','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','W9','该银行卡交易支付次数被限制','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','YC','该终端黑名单下不能判断该银行卡的卡种','\u672a\u77e5\u5361','\u672a\u77e5\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','YE','该终端下不能判断该银行卡的卡种','\u672a\u77e5\u5361','\u672a\u77e5\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','YF','该终端下借记卡交易支付的次数超限','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','YG','该终端下信用卡交易支付的次数超限','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','YH','该终端下同一借记卡卡号支付次数超限','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','YI','该终端下同一信用卡卡号支付次数超限','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','YJ','该终端下对银行卡号支付次数限制','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','YK','该终端下对所有银行卡号支付次数限制','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XE','该商户黑名单下不能判断该银行卡的卡种','\u672a\u77e5\u5361','\u672a\u77e5\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XF','该商户下不能判断该银行卡的卡种','\u672a\u77e5\u5361','\u672a\u77e5\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XG','该商户下借记卡交易支付次数超限','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XH','该商户下信用卡交易支付次数超限','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XI','该商户下同一借机卡卡号交易支付次数超限','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XJ','该商户下同一信用卡卡号交易支付次数超限','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XK','该商户下对银行卡交易支付次数限制','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XL','该商户下对所有银行卡交易支付次数限制','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Z6','该渠道下不能判断该银行卡的卡种','\u672a\u77e5\u5361','\u672a\u77e5\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Z7','该渠道下信用卡日交易金额超限','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Z8','该渠道下借记卡日交易金额超限','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','P1','该商户下 限制银行卡进行交易','\u53d7\u9650\u5236\u7684\u5361','\u53d7\u9650\u5236\u7684\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','P2','该商户下 限制银行卡交易次数限制','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','P3','该商户下 不能低于该银行卡的最低金额','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','P4','该商户下 不能高于该银行卡的最高金额','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','P5','该商户下 不能低于该银行卡日交易最低','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','P6','该商户下 不能高于该银行卡日交易最高金额','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','P7','该商户下 该银行卡黑名单不能交易','\u4ea4\u6613\u9650\u5236','\u4ea4\u6613\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','P8','该商户下 该银行卡黑名单单笔金额超限','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','P9','该商户下 该银行卡黑名单日交易金额超限','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','PA','该终端下 该银行卡黑名单不能交易','\u4ea4\u6613\u9650\u5236','\u4ea4\u6613\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','PB','该终端下 该银行卡黑名单单笔金额超限','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','PC','该终端下 该银行卡黑名单日交易金额超限','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','PE','该终端下 限制银行卡进行交易','\u53d7\u9650\u5236\u7684\u5361','\u53d7\u9650\u5236\u7684\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','PF','该终端下 限制银行卡交易次数限制','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','PG','该终端下 不能低于该银行卡的最低金额','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','PH','该终端下 不能高于该银行卡的最高金额','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','PI','该终端下 不能低于该银行卡日交易最低金额','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','PJ','该终端下 不能高于该银行卡日交易最高金额','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','G1','跨行转账汇款 该转出卡单笔金额超限','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','G2','跨行转账汇款 该转出卡日交易金额超限','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','G3','跨行转账汇款 该转出卡交易次数超限','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','G4','ATM转账汇款 该转出卡单笔金额超限','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','G5','ATM转账汇款 该转出卡日交易金额超限','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','G6','ATM转账汇款 该转出卡交易次数超限','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');

-- lp by leep
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','G7','跨行转账汇款 该金额配置路由不存在','\u91d1\u989d\u4e0d\u5408\u6cd5','\u91d1\u989d\u4e0d\u5408\u6cd5');

-- 湖北供销社转账汇款特殊交易应答处理
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('208','A6','转账汇款超时,请向客服确认交易。','\u8f6c\u8d26\u6c47\u6b3e\u8d85\u65f6,\u8bf7\u5411\u5ba2\u670d\u786e\u8ba4\u4ea4\u6613\u3002','\u8f6c\u8d26\u6c47\u6b3e\u8d85\u65f6,\u8bf7\u5411\u5ba2\u670d\u786e\u8ba4\u4ea4\u6613\u3002');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('208','59','卡片检验失败。','\u5361\u7247\u68c0\u9a8c\u5931\u8d25\u3002','\u5361\u7247\u68c0\u9a8c\u5931\u8d25\u3002');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('208','36','转出卡受限制。','\u8f6c\u51fa\u5361\u53d7\u9650\u5236\u3002','\u8f6c\u51fa\u5361\u53d7\u9650\u5236\u3002');