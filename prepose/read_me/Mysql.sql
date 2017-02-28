-- �������ݿ�
drop database IF EXISTS PEP;
create database PEP;
use PEP;


-- CURRVAL ʵ�ַ���
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

-- nextval ʵ�ַ���
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

-- setval ʵ�ַ���
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

-- �÷�ʵ��
-- ��ѯ��ǰtrace_seq��ֵ
-- select currval('trace_seq'); 

-- ��ѯ��һ��trace_seq��ֵ
-- select nextval('trace_seq');  

-- ���õ�ǰtrace_seq��ֵ
-- select setval('trace_seq',150);  




-- �̻���������
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

-- ����EWP���ͱ�����Ϣ������Ҫ����Ϣ
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

-- ����������Ϣ��
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
  
insert into MSGCHG_INFO(trancde, msgtype, process, service, amtcode, mercode, termcde, revoid, remark, amtmin, flagpwd, flagpe) values('101','0200','910000','00','156','860010030210001','12345678','0','��Ʊ����ӰƱ����֧��','0','1','Y');
insert into MSGCHG_INFO(trancde, msgtype, process, service, amtcode, mercode, termcde, revoid, remark, amtmin, flagpwd, flagpe) values('102','0200','910000','00','156','860010030210002','12345678','0','����ͨ������Ʊ����֧��','0','1','Y');
insert into MSGCHG_INFO(trancde, msgtype, process, service, amtcode, mercode, termcde, revoid, remark, amtmin, flagpwd, flagpe) values('103','0200','910000','00','156','860010030210003','12345678','0','�������ջ�Ʊ����֧��','0','1','Y');
insert into MSGCHG_INFO(trancde, msgtype, process, service, amtcode, mercode, termcde, revoid, remark, amtmin, flagpwd, flagpe) values('104','0200','910000','00','156','860010030210004','12345678','0','�·���������Ϸ��ֵ����֧��','0','1','Y');
insert into MSGCHG_INFO(trancde, msgtype, process, service, amtcode, mercode, termcde, revoid, remark, amtmin, flagpwd, flagpe) values('105','0200','910000','00','156','860010030210005','12345678','0','��Ԫ����Ϸ�㿨����֧��','0','1','Y');
insert into MSGCHG_INFO(trancde, msgtype, process, service, amtcode, mercode, termcde, revoid, remark, amtmin, flagpwd, flagpe) values('106','0200','910000','00','156','860010030210006','12345678','0','��ͨ�����֧��','0','1','Y');
insert into MSGCHG_INFO(trancde, msgtype, process, service, amtcode, mercode, termcde, revoid, remark, amtmin, flagpwd, flagpe) values('107','0200','910000','00','156','860010030210007','12345678','0','ˮ��ú����֧��','0','1','Y');
commit;

-- ǰ��ϵͳ���׼�¼��
drop table IF EXISTS PEP_JNLS;
create table PEP_JNLS
(
  pepdate 	     CHAR(8) not null,  -- ǰ��ϵͳ������ ���ĵ�7��
  peptime        CHAR(6) not null,  -- ǰ��ϵͳ��ʱ�� ���ĵ�7��
  trancde        CHAR(3) not null,  -- ������
  msgtype        CHAR(4) not null,  -- ��Ϣ����
  outcdno        VARCHAR(20),          -- ת������
  outcdtype      VARCHAR(1),           -- ת��������
  intcdno        VARCHAR(40),          -- ת�뿨��
  intcdbank 	 VARCHAR(30),	-- ����ת�˻��׵�ת�������б��
  intcdname 	 VARCHAR(30),	-- ����ת�˻��׵�ת�뿨����
  process        CHAR(6) not null,  -- ���״�����
  tranamt        VARCHAR(12),          -- ���׽��
  trace          numeric(6) not null, -- ǰ��ϵͳ��ˮ
  termtrc        VARCHAR(6),            -- �ն���ˮ
  trntime        CHAR(6) not null,   -- ewp������ʱ��/pos������ʱ��
  trndate        CHAR(8) not null,   -- ewp����������/pos����������
  poitcde        VARCHAR(3),
  track2         VARCHAR(37),          -- ����
  track3         VARCHAR(104),         -- ����
  sendcde        VARCHAR(8),			-- �ն˱�ʾ�� 01000000(�ֻ�),02000000(����ƽ̨),03000000(POS),04000000(sk ewp)
  syseqno        VARCHAR(12),          -- �ο���
  termcde        VARCHAR(8) not null,  -- �ڲ��ն˺�
  mercode        VARCHAR(15) not null, -- �ڲ��̻���
  billmsg        VARCHAR(40) not null, -- ������/DY+����(pepdate,8)+ʱ��(peptime,6)+ǰ����ˮ(trace,6)
  samid          VARCHAR(20) not null, -- psam����/POS���޸��ֶ���Ϣ
  termid         VARCHAR(25),          -- �绰����/APN����
  termidm        VARCHAR(20),			-- �ն�������,�ֻ�/POS��Ӳ�����
  gpsid			 VARCHAR(35),			-- ����POS��վ����
  aprespn        VARCHAR(2),           -- Ӧ����
  sdrespn        VARCHAR(2),
  revodid        VARCHAR(1),
  billflg        VARCHAR(1),
  mmchgid        VARCHAR(1),
  mmchgnm        numeric(16),
  termmac        VARCHAR(16),
  termpro        VARCHAR(200),
  trnsndpid      numeric(4),			 -- Folder ID 
  trnsndp        VARCHAR(15),       -- folder����
  trnmsgd        VARCHAR(4),        -- TPDUԴ��ַ
  revdate        VARCHAR(10),  
  modeflg        CHAR(1),            -- �Ƿ��Ͷ��ŵı�ʾ
  modecde        VARCHAR(24),       -- �û����ն�Ϣ���ֻ�����
  filed28        VARCHAR(9),            -- ����������
  filed48        VARCHAR(256),
  merid          numeric(5),         -- vasƽ̨���̻���
  authcode       VARCHAR(6),            -- ��Ȩ��
  discountamt    VARCHAR(12),      -- EWPϵͳ�Żݽ��
  poscode        VARCHAR(2),
  translaunchway VARCHAR(1),
  batchno        VARCHAR(6),			 -- �������κ�
  camtlimit      numeric(1),			 -- ���ÿ����޶��жϱ�ʶ
  damtlimit      numeric(1),			 -- ��ǿ����޶��жϱ�ʶ
  termmeramtlimit      numeric(1),			 -- �ն��̻����ÿ����޶��жϱ�ʶ
  settlementdate  VARCHAR(4),			-- ��������MMDD,����15��
  reversedflag    VARCHAR(1),			-- �Ƿ��������ı�ʶ 1���ն˷������  0��δ�������
  reversedans     VARCHAR(2),			-- ����Ӧ��
  sysreno         VARCHAR(12),			-- �ն˳������ص�ϵͳ�ο���
  postermcde      VARCHAR(8),			-- pos�ն����������̻���
  posmercode      VARCHAR(15),			-- pos�ն����ϵ��ն˺�
  filed44 		VARCHAR(22) -- ������Ӧ�����ڽ�����Ӧ��Ϣ���м���пո�ķ��ؽ��ջ������յ������ı�ʶ��
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
-- alter table PEP_JNLS add INTCDBANK VARCHAR(30);	-- ����ת�˻��׵�ת�������б��
-- alter table PEP_JNLS add INTCDNAME VARCHAR(30);	-- ����ת�˻��׵�ת�뿨����

commit;

drop table IF EXISTS PEP_JNLS_ALL;
create table PEP_JNLS_ALL
(
  pepdate        CHAR(8) not null,  -- ǰ��ϵͳ������ ���ĵ�7��
  peptime        CHAR(6) not null,  -- ǰ��ϵͳ��ʱ�� ���ĵ�7��
  trancde        CHAR(3) not null,  -- ������
  msgtype        CHAR(4) not null,  -- ��Ϣ����
  outcdno        VARCHAR(20),          -- ת������
  outcdtype      VARCHAR(1),           -- ת��������
  intcdno        VARCHAR(40),          -- ת�뿨��
  intcdbank    VARCHAR(30), -- ����ת�˻��׵�ת�������б��
  intcdname    VARCHAR(30), -- ����ת�˻��׵�ת�뿨����
  process        CHAR(6) not null,  -- ���״�����
  tranamt        VARCHAR(12),          -- ���׽��
  trace          numeric(6) not null, -- ǰ��ϵͳ��ˮ
  termtrc        VARCHAR(6),            -- �ն���ˮ
  trntime        CHAR(6) not null,   -- ewp������ʱ��/pos������ʱ��
  trndate        CHAR(8) not null,   -- ewp����������/pos����������
  poitcde        VARCHAR(3),
  track2         VARCHAR(37),          -- ����
  track3         VARCHAR(104),         -- ����
  sendcde        VARCHAR(8),      -- �ն˱�ʾ�� 01000000(�ֻ�),02000000(����ƽ̨),03000000(POS),04000000(sk ewp)
  syseqno        VARCHAR(12),          -- �ο���
  termcde        VARCHAR(8) not null,  -- �ڲ��ն˺�
  mercode        VARCHAR(15) not null, -- �ڲ��̻���
  billmsg        VARCHAR(40) not null, -- ������/DY+����(pepdate,8)+ʱ��(peptime,6)+ǰ����ˮ(trace,6)
  samid          VARCHAR(20) not null, -- psam����/POS���޸��ֶ���Ϣ
  termid         VARCHAR(25),          -- �绰����/APN����
  termidm        VARCHAR(20),     -- �ն�������,�ֻ�/POS��Ӳ�����
  gpsid      VARCHAR(35),     -- ����POS��վ����
  aprespn        VARCHAR(2),           -- Ӧ����
  sdrespn        VARCHAR(2),
  revodid        VARCHAR(1),
  billflg        VARCHAR(1),
  mmchgid        VARCHAR(1),
  mmchgnm        numeric(16),
  termmac        VARCHAR(16),
  termpro        VARCHAR(200),
  trnsndpid      numeric(4),       -- Folder ID 
  trnsndp        VARCHAR(15),       -- folder����
  trnmsgd        VARCHAR(4),        -- TPDUԴ��ַ
  revdate        VARCHAR(10),  
  modeflg        CHAR(1),            -- �Ƿ��Ͷ��ŵı�ʾ
  modecde        VARCHAR(24),       -- �û����ն�Ϣ���ֻ�����
  filed28        VARCHAR(9),            -- ����������
  filed48        VARCHAR(256),
  merid          numeric(5),         -- vasƽ̨���̻���
  authcode       VARCHAR(6),            -- ��Ȩ��
  discountamt    VARCHAR(12),      -- EWPϵͳ�Żݽ��
  poscode        VARCHAR(2),
  translaunchway VARCHAR(1),
  batchno        VARCHAR(6),       -- �������κ�
  camtlimit      numeric(1),       -- ���ÿ����޶��жϱ�ʶ
  damtlimit      numeric(1),       -- ��ǿ����޶��жϱ�ʶ
  termmeramtlimit      numeric(1),       -- �ն��̻����ÿ����޶��жϱ�ʶ
  settlementdate  VARCHAR(4),     -- ��������MMDD,����15��
  reversedflag    VARCHAR(1),     -- �Ƿ��������ı�ʶ 1���ն˷������  0��δ�������
  reversedans     VARCHAR(2),     -- ����Ӧ��
  sysreno         VARCHAR(12),      -- �ն˳������ص�ϵͳ�ο���
  postermcde      VARCHAR(8),     -- pos�ն����������̻���
  posmercode      VARCHAR(15),      -- pos�ն����ϵ��ն˺�
  filed44     VARCHAR(22) -- ������Ӧ�����ڽ�����Ӧ��Ϣ���м���пո�ķ��ؽ��ջ������յ������ı�ʶ��
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
-- alter table PEP_JNLS add INTCDBANK VARCHAR(30);  -- ����ת�˻��׵�ת�������б��
-- alter table PEP_JNLS add INTCDNAME VARCHAR(30);  -- ����ת�˻��׵�ת�뿨����

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


-- pos�����ͱ���ϸ
drop table IF EXISTS pos_batch_send_detail_jnls;
create table pos_batch_send_detail_jnls
(
  id				   int not null, -- ����id ������
  pid				   int not null, -- ����pos_settlement_detail_jnls������Լ��
  trace				   numeric(6) not null,  -- ��ˮ
  cardno			   VARCHAR(19),  -- ���п���
  amt				   VARCHAR(12),  -- ���׽��
  reference			   VARCHAR(12),  -- �ο���
  pos_mercode          VARCHAR(15) not null,  -- �ڲ��̻���
  pos_termcode         VARCHAR(8) not null, -- �ڲ��ն˺�
  batch_no             numeric(6) not null,	-- ���κ�
  reslut			   VARCHAR(2) not null		-- Ӧ����,39��
)DEFAULT CHARSET=utf8;

alter table pos_batch_send_detail_jnls add constraint pos_batch_send_detail_jnls_pk primary key (id);

-- create sequence pos_batchsend_detail_sequence
-- minvalue 1
-- maxvalue 999999999
-- start with 1
-- increment by 1
-- cache 50;

-- pos�����ͱ���ϸ���к�
DROP TABLE IF EXISTS pos_batchsend_detail_sequence;
CREATE TABLE pos_batchsend_detail_sequence (
    name              VARCHAR(50) NOT NULL,
    current_value INT NOT NULL,
    increment       INT NOT NULL DEFAULT 1,
    PRIMARY KEY (name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
INSERT INTO pos_batchsend_detail_sequence VALUES ('trace_detail_seq',1,1);

commit;

-- POS_CHANNEL_INFO ��
drop table IF EXISTS POS_CHANNEL_INFO;
create table POS_CHANNEL_INFO
(
  mgr_term_id VARCHAR(8) not null, -- pos�ն˺�
  mgr_mer_id  VARCHAR(15) not null, -- pos�̻���
  channel_id  numeric(4) not null  -- �ն�pos��������ID
)DEFAULT CHARSET=utf8;
--  Create/Recreate primary, unique and foreign key constraints 
alter table POS_CHANNEL_INFO
  add constraint POS_CHANNEL_INFOPK primary key (MGR_TERM_ID, MGR_MER_ID, CHANNEL_ID);
  
commit;

-- pos����������Ϣ��
drop table IF EXISTS POS_CONF_INFO;
create table POS_CONF_INFO
(
  mgr_term_id      VARCHAR(8) not null,  -- �ն˺�
  mgr_mer_id       VARCHAR(15) not null, -- �̻���
  mgr_flag         VARCHAR(1),  -- �̻���ʶ��ǩ���Ƿ�ɹ���ʾ��Y��ǩ���ɹ���N��ǩ��ʧ��
  mgr_mer_des      VARCHAR(40), -- �̻�����
  mgr_batch_no     VARCHAR(6),	-- ���κ�
  mgr_folder       VARCHAR(14), -- ��·����
  mgr_tpdu         VARCHAR(22), -- TPDUͷ
  mgr_signout_flag numeric(1), -- ǩ�˱�ʾ��1���Զ���0�����Զ�
  mgr_key_index    numeric(4), -- ��ǰʹ������Կ����ֵ
  mgr_kek          VARCHAR(32), -- kek
  mgr_desflg       VARCHAR(3), -- ���ܷ�����DES��001 3DES��003  �ŵ���Կ��004
  mgr_pik          VARCHAR(32), -- pik������Կ
  mgr_mak          VARCHAR(16), -- mak������Կ
  mgr_tdk          VARCHAR(32), -- �ŵ�������Կ
  mgr_signtime     VARCHAR(14),  -- YYYYMMDD hhmmss
  track_pwd_flag   VARCHAR(2),  -- �Ƿ��дŵ�������ı��  00 �޴����� 01 �޴�����  10 �д�����  11 �д�����
  pos_machine_id   VARCHAR(20) not null,  -- pos ���߱��
  pos_machine_type int not null, -- 2:�����ƶ�֧���ն�3:����pos,4:����gprspos,5:����apn��pos,6:sk��ĿPOS(һ���),7:USB POS(������Ŀ)
  pos_apn          VARCHAR(30), -- apn����
  pos_telno        VARCHAR(16), -- �绰����
  pos_gpsno        VARCHAR(35),  -- ��λ����
  pos_type		   VARCHAR(2), --  pos�ն�Ӧ������Ĭ��ֵ60
  cz_retry_count   numeric(1),  -- �����ط����� Ĭ��ֵ3��
  trans_telno1	   VARCHAR(14),  -- �������׵绰����֮һ,14λ����,�Ҳ��ո�
  trans_telno2	   VARCHAR(14),  -- �������׵绰����֮��,14λ����,�Ҳ��ո�
  trans_telno3	   VARCHAR(14),  -- �������׵绰����֮��,14λ����,�Ҳ��ո�
  manager_telno	   VARCHAR(14),  -- һ������绰����,14λ����
  hand_flag		   numeric(1),  -- �Ƿ�֧���ֹ����뿨�� 1:֧��,0:��֧��(Ĭ�ϲ�֧��)	
  trans_retry_count   numeric(1),  -- �����ط�����  Ĭ��3��
  pos_main_key_index  VARCHAR(2),  -- �ն�����Կ����ֵ Ĭ��Ϊ01
  pos_ip_port1     VARCHAR(30),  -- ����POS IP1 �Ͷ˿ں�1
  pos_ip_port2     VARCHAR(30),  -- ����POS IP2 �Ͷ˿ں�2
  rtn_track_pwd_flag  VARCHAR(2), -- �����˻��Ƿ���Ҫ�ŵ������� 10:�дŵ�������
  mgr_timeout   numeric(2), -- ��ʱʱ�� Ĭ��60��
  input_order_flg VARCHAR(2),-- �ն��Ƿ��ж������������ı��.01:�ն˴������붩���Ž��� 00:�ն˲��������붩���ŵĽ���Ĭ����00
  pos_update_flag    numeric(1) default 0,
  pos_update_add     VARCHAR(30) -- POSӦ�ó�����µĵ�ַIP�Ͷ˿�
)DEFAULT CHARSET=utf8;

--  Create/Recreate primary, unique and foreign key constraints 
alter table POS_CONF_INFO
  add constraint POSCONFINFO_PK primary key (MGR_TERM_ID, MGR_MER_ID, POS_MACHINE_ID);
commit;

-- �����ն˸����յ��̻�������Ϣ��
drop table IF EXISTS pos_cust_info;
create table pos_cust_info
(
  cust_id      		  VARCHAR(20) not null,  -- �û����룬�����ֻ������
  cust_psam			  VARCHAR(16) not null,  -- psam����
  cust_merid          VARCHAR(15) not null, -- �ڲ��̻���
  cust_termid         VARCHAR(8) not null,  -- �ڲ��ն˺�
  cust_incard         VARCHAR(23) not null, -- ת�뿨��
  cust_ebicode        VARCHAR(20) not null, -- �����ʺ�
  cust_merdescr       VARCHAR(50) not null,	-- �̻�����
  cust_merdescr_utf8  VARCHAR(200) not null,		-- �̻����Ƶ�UTF_8�ĸ�ʽ
  cust_coraccountno	  VARCHAR(23), -- �Թ��˻���
  cust_updatetime     timestamp(6)   -- YYYY-MM-DD HH24:MI:SS
)DEFAULT CHARSET=utf8;
alter table pos_cust_info add constraint POS_CUST_INFO_PK primary key (CUST_PSAM);
alter table pos_cust_info add constraint POS_CUST_ID_UQ unique (CUST_ID);

-- pos�����ͱ�
drop table IF EXISTS pos_settlement_detail_jnls;
create table pos_settlement_detail_jnls
(
  id				   int not null, -- ����id
  pid				   int not null, -- ���� ��pos_settlement_jnls�����Լ��
  trace				   numeric(6) not null, -- ��ˮ
  reference			   VARCHAR(12) not null,  -- �ο���
  pos_mercode          VARCHAR(15) not null,  -- �ڲ��̻���
  pos_termcode         VARCHAR(8) not null, -- �ڲ��ն˺�
  batch_no             numeric(6) not null,	-- ���κ� 60.2
  reslut			   VARCHAR(2) not null,		-- Ӧ����,39��
  netmanager_code		VARCHAR(3) not null,		-- ���������Ϣ�룬60.3 
  package_count	       int not null, -- ��ǰ���İ��������ܱ��� ��48���а������׵ı�����������ͽ���ʱ�������͵��ܱ���
  pep_jnls_count	   int, -- ʵ�ʲ�ѯ��õĽ����ܱ���
  success_count	       int, -- ʵ�ʳɹ��ܱ���
  failed_count	       int, -- ʵ��ʧ���ܱ��� 
  bit48	               VARCHAR(322), -- �����ͱ���48��ԭʼ��Ϣ
  senddate      	   VARCHAR(8) not null,  -- ǰ������
  sendtime             VARCHAR(6) not null -- ǰ��ʱ��
)DEFAULT CHARSET=utf8;


--  create sequence pos_batch_send_sequence
--  minvalue 1
--  maxvalue 999999999
--  start with 1
--  increment by 1
--  cache 50;

alter table pos_settlement_detail_jnls add constraint pos_settlement_detail_jnls_pk primary key (id);

-- pos�������
drop table IF EXISTS pos_settlement_jnls;
create table pos_settlement_jnls
(
  id				   int not null,    -- ������������
  xpepdate      	   VARCHAR(8) not null,  -- ǰ������
  xpeptime             VARCHAR(6) not null, -- ǰ��ʱ��
  trace				   numeric(6) not null,
  reference			   VARCHAR(12) not null,  -- �ο���
  pos_mercode          VARCHAR(15) not null,  -- �ڲ��̻���
  pos_termcode         VARCHAR(8) not null, -- �ڲ��ն˺�
  settlement_date      VARCHAR(4) not null, -- ��������
  batch_no             numeric(6) not null,	-- ���κ�
  operator_no		   VARCHAR(3) not null,		-- ����Ա����
  pos_jieji_amount	   VARCHAR(12), -- ����ܽ�� 
  pos_jieji_count	   int, -- ����ܱ��� 
  pos_daiji_amount	   VARCHAR(12), -- �����ܽ�� 
  pos_daiji_count	   int, -- �����ܱ��� 
  xpep_jieji_amount	   VARCHAR(12), -- xpep����ܽ�� 
  xpep_jieji_count	   int, -- xpep����ܱ��� 
  xpep_daiji_cx_amount	   VARCHAR(12), -- xpep����(����)�ܽ�� 
  xpep_daiji_cx_count	   int, -- xpep����(����)�ܱ���
  xpep_daiji_th_amount	   VARCHAR(12), -- xpep����(�˻�)�ܽ�� 
  xpep_daiji_th_count	   int, -- xpep����(�˻�)�ܱ���
  settlement_result	   int,  -- ����Ӧ����
  termidm   	VARCHAR(20)	-- �ն�������,�ֻ�/POS��Ӳ����� ���߱��
)DEFAULT CHARSET=utf8;

-- create sequence pos_settlement_jnls_sequence
-- minvalue 1
-- maxvalue 999999999
-- start with 1
-- increment by 1
-- cache 50;

-- alter table pos_settlement_jnls add constraint pos_settlement_jnls_ primary key (xpepdate,xpeptime,trace,pos_mercode,pos_termcode);
alter table pos_settlement_jnls add constraint pos_settlement_jnls_pk primary key (id);

-- pos����������к�
DROP TABLE IF EXISTS pos_settlement_jnls_sequence;
CREATE TABLE pos_settlement_jnls_sequence (
    name              VARCHAR(50) NOT NULL,
    current_value INT NOT NULL,
    increment       INT NOT NULL DEFAULT 1,
    PRIMARY KEY (name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
INSERT INTO pos_settlement_jnls_sequence VALUES ('trace_settlement_seq',1,1);


-- �ն�Ͷ�ʷ���Ϣ�������Ϻ����ž���һ���ն�Ͷ�ʷ���
-- �ն�Ͷ�ʷ���ָ��psam����˭��ǮͶ�ʵġ�
drop table IF EXISTS samcard_channel_info;
create table samcard_channel_info
(
  channel_id       numeric(4) not null,         -- �ն�Ͷ�ʷ������ı��
  channel_name     VARCHAR(50) not null,      -- �ն�Ͷ�ʷ�����
  restrict_trade   numeric(1) default 0 not null,      -- �ն�Ͷ�ʷ��������Ʊ�ʾ��Ĭ��Ϊ0���������ơ����Ϊ1����ô�������ƽ��ס�
  channelkey_index	numeric(3) -- ��������Ӧ����Կ����
)DEFAULT CHARSET=utf8;

alter table samcard_channel_info
  add constraint samcard_channel_info_PKEY primary key (channel_id);
  
  insert into samcard_channel_info(channel_id, channel_name, restrict_trade)values(1,'����',0);
  commit;  

-- psam����Ϣ��
drop table IF EXISTS SAMCARD_INFO;
create table SAMCARD_INFO
(
  sam_card       VARCHAR(16) not null,   -- psam����
  channel_id     numeric(4) default 0 not null,      -- psam������������
  sam_model      VARCHAR(1) not null,    -- psam����
  sam_status     VARCHAR(2) not null,    -- psam��״̬
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

-- �ն��̻����Ʊ���ʾ���Ƹ��ն���ĳ���̻������ÿ����޶�����
drop table IF EXISTS samcard_mertrade_restrict;
create table samcard_mertrade_restrict
(
  psam_card        VARCHAR(16) not null,  -- psam����
  mercode		   VARCHAR(15) not null,  -- �̻���
  c_amttotal       char(12) default '-',  -- Ĭ��-������
  c_amtdate        VARCHAR(8), -- ��ʾĳ��������
  c_total_tranamt  char(12) default '000000000000' -- ���ÿ����ս����ܶ�
)DEFAULT CHARSET=utf8;

alter table samcard_mertrade_restrict
  add constraint samcardmertraderestrictpk primary key (psam_card, mercode);
  
  insert into samcard_mertrade_restrict(psam_card, mercode, c_amtdate)values('CADE000012345678','860010030210001','20120605');
  insert into samcard_mertrade_restrict(psam_card, mercode, c_amtdate)values('CADE000012345678','860010030210002','20120605');
  insert into samcard_mertrade_restrict(psam_card, mercode, c_amtdate)values('CADE000012345678','860010030210003','20120605');
  commit;  


-- psam��״̬����Ӧ��SAMCARD_INFO���sam_status�ֶ�
drop table IF EXISTS samcard_status;
create table samcard_status
(
    samcard_status_code       VARCHAR(2) not null,
	samcard_status_desc		  VARCHAR(25) not null,
	xpep_ret_code       VARCHAR(2) not null
)DEFAULT CHARSET=utf8;

alter table samcard_status
  add constraint samcard_status_pk primary key (samcard_status_code);

insert into samcard_status values('00','����','00');
insert into samcard_status values('01','�ն������','FR');
insert into samcard_status values('02','�ն˳�����','FS');
insert into samcard_status values('03','�ն�ͣ����','FT');
insert into samcard_status values('04','�ն˸�����','FU');
insert into samcard_status values('05','�ն�δʹ��','FV');
insert into samcard_status values('06','�ն�ʹ����','FW');
insert into samcard_status values('07','�ն���ͣ��','FX');

commit;

-- �ն����Ʊ����ڸ�psam���Ϸ����Ľ��׶���Ҫ������
drop table IF EXISTS SAMCARD_TRADE_RESTRICT;
create table SAMCARD_TRADE_RESTRICT
(
  psam_card      VARCHAR(16) not null,
  restrict_trade VARCHAR(27) default '-',
  restrict_card  char(1) default '-',
  passwd         char(1) default '-',
  amtmin         char(12) default '-', -- ������С���
  amtmax         char(12) default '-', -- ���������
  c_amttotal       char(12) default '-',-- ���ÿ����޶��ж�
  c_amtdate        VARCHAR(8),
  c_total_tranamt  char(12) default '000000000000',
  d_amttotal       char(12) default '-',-- ��ǿ����޶��ж�
  d_amtdate        VARCHAR(8),
  d_total_tranamt  char(12) default '000000000000',
  fail_count     numeric(4) default -1,
  success_count  numeric(4) default -1
)DEFAULT CHARSET=utf8;

alter table SAMCARD_TRADE_RESTRICT
  add constraint SAMCARD_TRADE_RESTRICT_pk primary key (psam_card);
  
insert into SAMCARD_TRADE_RESTRICT(psam_card)values('CADE000012345678');

commit;

-- �ն�Ͷ�ʷ��������Ʊ�
drop table IF EXISTS samcardchannel_to_msgchginfo;
create table samcardchannel_to_msgchginfo
(
  channel_id       numeric(4) not null,       -- �ն�Ͷ�ʷ������ı��
  trancde          CHAR(3) not null,     -- ������
  restrict_trade   numeric(1)	default 0 not null     -- �������Ʊ�ʾ��Ĭ��Ϊ0���������ơ����Ϊ1����ô�������ƽ��ס�
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

-- sms_jnls ��
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

-- xpep_errtrade_jnls��
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


-- MySQL ������������ʵ��

-- ǰ�ý�����ˮ��
DROP TABLE IF EXISTS xpep_jnls_trace_sequence;
CREATE TABLE xpep_jnls_trace_sequence (
    name              VARCHAR(50) NOT NULL,
    current_value INT NOT NULL,
    increment       INT NOT NULL DEFAULT 1,
    PRIMARY KEY (name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
INSERT INTO xpep_jnls_trace_sequence VALUES ('trace_seq',1,1);


--  ǰ��ϵͳӦ�������������Ӧ�����������Ӧ���룬��Ӧ���������
drop table IF EXISTS xpep_retcode;
create table xpep_retcode
(
  xpep_trancde   CHAR(3) not null,  -- ������
  xpep_retcode   CHAR(2) not null,  -- ����Ӧ����
  xpep_content   VARCHAR(150) not null, -- ����Ӧ��������
  xpep_content_ewp   VARCHAR(255) not null, -- ����EWP����Ӧ��������Ϣ��utf-8
  xpep_content_ewp_print   VARCHAR(500) not null, -- ����EWP�Ĵ�ӡ���ݣ�utf-8
  xpep_desc      VARCHAR(160)       -- ����������Ϣ
)DEFAULT CHARSET=utf8;
alter table xpep_retcode
  add constraint xpep_retcode_pk primary key (xpep_trancde, xpep_retcode);

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
-- -- -- -- -- -- add by he.qingqi  8/31   -- -- -- -
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','00','�жһ��׳ɹ�','\u627f\u5151\u6216\u4ea4\u6613\u6210\u529f','\u627f\u5151\u6216\u4ea4\u6613\u6210\u529f');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','01','�鷢����','\u67e5\u53d1\u5361\u884c','\u67e5\u53d1\u5361\u884c');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','02','�鷢���е���������','\u53ef\u7535\u8bdd\u5411\u53d1\u5361\u884c\u67e5\u8be2','\u53ef\u7535\u8bdd\u5411\u53d1\u5361\u884c\u67e5\u8be2');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','03','��Ч�̻�','\u5546\u6237\u9700\u8981\u5728\u94f6\u884c\u6216\u4e2d\u5fc3\u767b\u8bb0','\u5546\u6237\u9700\u8981\u5728\u94f6\u884c\u6216\u4e2d\u5fc3\u767b\u8bb0');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','04','û�տ�','\u64cd\u4f5c\u5458\u6ca1\u6536\u5361','\u64cd\u4f5c\u5458\u6ca1\u6536\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','05','����ж�','\u53d1\u5361\u4e0d\u4e88\u627f\u5151','\u53d1\u5361\u4e0d\u4e88\u627f\u5151');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','06','����','\u53d1\u5361\u884c\u6545\u969c','\u53d1\u5361\u884c\u6545\u969c');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','07','����������û�տ�','\u7279\u6b8a\u6761\u4ef6\u4e0b\u6ca1\u6536\u5361','\u7279\u6b8a\u6761\u4ef6\u4e0b\u6ca1\u6536\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','08','�������ڴ�����','\u91cd\u65b0\u63d0\u4ea4\u4ea4\u6613\u8bf7\u6c42','\u91cd\u65b0\u63d0\u4ea4\u4ea4\u6613\u8bf7\u6c42');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','09','��Ч����','\u53d1\u5361\u884c\u4e0d\u652f\u6301\u7684\u4ea4\u6613','\u53d1\u5361\u884c\u4e0d\u652f\u6301\u7684\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','12','��Ч����','\u65e0\u6548\u4ea4\u6613','\u65e0\u6548\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','13','��Ч���','\u91d1\u989d\u4e3a0\u6216\u592a\u5927','\u91d1\u989d\u4e3a0\u6216\u592a\u5927');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','14','��Ч����','\u5361\u79cd\u672a\u5728\u4e2d\u5fc3\u767b\u8bb0\u6216\u8bfb\u5361\u53f7\u6709\u8bef','\u5361\u79cd\u672a\u5728\u4e2d\u5fc3\u767b\u8bb0\u6216\u8bfb\u5361\u53f7\u6709\u8bef');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','15','�޴˷�����','\u6b64\u53d1\u5361\u884c\u672a\u4e0e\u4e2d\u5fc3\u5f00\u901a\u4e1a\u52a1','\u6b64\u53d1\u5361\u884c\u672a\u4e0e\u4e2d\u5fc3\u5f00\u901a\u4e1a\u52a1');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','16','��׼���µ����ŵ�','\u6279\u51c6\u66f4\u65b0\u7b2c\u4e09\u78c1\u9053','\u6279\u51c6\u66f4\u65b0\u7b2c\u4e09\u78c1\u9053');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','19','�������뽻��','\u5237\u5361\u8bfb\u53d6\u6570\u636e\u6709\u8bef\uff0c\u53ef\u91cd\u65b0\u5237\u5361','\u5237\u5361\u8bfb\u53d6\u6570\u636e\u6709\u8bef\uff0c\u53ef\u91cd\u65b0\u5237\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','20','��ЧӦ��','\u65e0\u6548\u5e94\u7b54','\u65e0\u6548\u5e94\u7b54');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','21','�����κδ���','\u4e0d\u505a\u4efb\u4f55\u5904\u7406','\u4e0d\u505a\u4efb\u4f55\u5904\u7406');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','22','���ɲ�������','POS\u72b6\u6001\u4e0e\u4e2d\u5fc3\u4e0d\u7b26\uff0c\u53ef\u91cd\u65b0\u7b7e\u5230','POS\u72b6\u6001\u4e0e\u4e2d\u5fc3\u4e0d\u7b26\uff0c\u53ef\u91cd\u65b0\u7b7e\u5230');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','23','���ɽ��ܵĽ��׷�','\u4e0d\u53ef\u63a5\u53d7\u7684\u4ea4\u6613\u8d39','\u4e0d\u53ef\u63a5\u53d7\u7684\u4ea4\u6613\u8d39');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','25','δ���ҵ��ļ��ϼ�¼','\u53d1\u5361\u884c\u672a\u80fd\u627e\u5230\u6709\u5173\u8bb0\u5f55','\u53d1\u5361\u884c\u672a\u80fd\u627e\u5230\u6709\u5173\u8bb0\u5f55');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','30','��ʽ����','\u683c\u5f0f\u9519\u8bef','\u683c\u5f0f\u9519\u8bef');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','31','������֧�ֵ�����','\u6b64\u53d1\u5361\u65b9\u672a\u4e0e\u4e2d\u5fc3\u5f00\u901a\u4e1a\u52a1','\u6b64\u53d1\u5361\u65b9\u672a\u4e0e\u4e2d\u5fc3\u5f00\u901a\u4e1a\u52a1');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','33','���ڵĿ�','\u8fc7\u671f\u7684\u5361\uff0c\u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536','\u8fc7\u671f\u7684\u5361\uff0c\u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','34','����������','\u6709\u4f5c\u5f0a\u5acc\u7591\u7684\u5361\uff0c\u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536','\u6709\u4f5c\u5f0a\u5acc\u7591\u7684\u5361\uff0c\u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','35','�ܿ����밲ȫ���ܲ�����ϵ','\u6709\u4f5c\u5f0a\u5acc\u7591\u7684\u5361\uff0c\u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536','\u6709\u4f5c\u5f0a\u5acc\u7591\u7684\u5361\uff0c\u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','36','�����ƵĿ�','\u8be5\u5361\u79cd\u4e0d\u53d7\u7406','\u8be5\u5361\u79cd\u4e0d\u53d7\u7406');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','37','�ܿ�����������ȫ���ܲ���(û�տ�)','\u6709\u4f5c\u5f0a\u5acc\u7591\u7684\u5361\uff0c\u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536','\u6709\u4f5c\u5f0a\u5acc\u7591\u7684\u5361\uff0c\u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','38','���������PIN������','\u5bc6\u7801\u9519\u6b21\u6570\u8d85\u9650\uff0c\u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536','\u5bc6\u7801\u9519\u6b21\u6570\u8d85\u9650\uff0c\u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','39','�޴����ÿ��˻�','\u53ef\u80fd\u5237\u5361\u64cd\u4f5c\u6709\u8bef','\u53ef\u80fd\u5237\u5361\u64cd\u4f5c\u6709\u8bef');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','40','����Ĺ����в�֧��','\u53d1\u5361\u884c\u4e0d\u652f\u6301\u7684\u4ea4\u6613\u7c7b\u578b','\u53d1\u5361\u884c\u4e0d\u652f\u6301\u7684\u4ea4\u6613\u7c7b\u578b');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','41','��ʧ��','\u6302\u5931\u7684\u5361\uff0c \u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536','\u6302\u5931\u7684\u5361\uff0c \u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','42','�޴��˻�','\u53d1\u5361\u884c\u627e\u4e0d\u5230\u6b64\u8d26\u6237','\u53d1\u5361\u884c\u627e\u4e0d\u5230\u6b64\u8d26\u6237');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','43','���Կ�','\u88ab\u7a83\u5361\uff0c \u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536','\u88ab\u7a83\u5361\uff0c \u64cd\u4f5c\u5458\u53ef\u4ee5\u6ca1\u6536');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','44','�޴�Ͷ���˻�','\u53ef\u80fd\u5237\u5361\u64cd\u4f5c\u6709\u8bef','\u53ef\u80fd\u5237\u5361\u64cd\u4f5c\u6709\u8bef');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','51','���㹻�Ĵ��','\u8d26\u6237\u5185\u4f59\u989d\u4e0d\u8db3','\u8d26\u6237\u5185\u4f59\u989d\u4e0d\u8db3');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','52','�޴�֧Ʊ�˻�','\u65e0\u6b64\u652f\u7968\u8d26\u6237','\u65e0\u6b64\u652f\u7968\u8d26\u6237');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','53','�޴˴���˻�','\u65e0\u6b64\u50a8\u84c4\u5361\u8d26\u6237','\u65e0\u6b64\u50a8\u84c4\u5361\u8d26\u6237');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','54','���ڵĿ�','\u8fc7\u671f\u7684\u5361','\u8fc7\u671f\u7684\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','55','����ȷ��PIN','\u5bc6\u7801\u8f93\u9519','\u5bc6\u7801\u8f93\u9519');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','56','�޴˿���¼','\u53d1\u5361\u884c\u627e\u4e0d\u5230\u6b64\u8d26\u6237','\u53d1\u5361\u884c\u627e\u4e0d\u5230\u6b64\u8d26\u6237');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','57','������ֿ��˽��еĽ���','\u4e0d\u5141\u8bb8\u6301\u5361\u4eba\u8fdb\u884c\u7684\u4ea4\u6613','\u4e0d\u5141\u8bb8\u6301\u5361\u4eba\u8fdb\u884c\u7684\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','58','�������ն˽��еĽ���','\u8be5\u5546\u6237\u4e0d\u5141\u8bb8\u8fdb\u884c\u7684\u4ea4\u6613','\u8be5\u5546\u6237\u4e0d\u5141\u8bb8\u8fdb\u884c\u7684\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','59','����������','\u6709\u4f5c\u5f0a\u5acc\u7591','\u6709\u4f5c\u5f0a\u5acc\u7591');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','60','�ܿ����밲ȫ���ܲ�����ϵ','\u53d7\u5361\u65b9\u4e0e\u5b89\u5168\u4fdd\u5bc6\u90e8\u95e8\u8054\u7cfb','\u53d7\u5361\u65b9\u4e0e\u5b89\u5168\u4fdd\u5bc6\u90e8\u95e8\u8054\u7cfb');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','61','����ȡ��������','\u4e00\u6b21\u4ea4\u6613\u7684\u91d1\u989d\u592a\u5927','\u4e00\u6b21\u4ea4\u6613\u7684\u91d1\u989d\u592a\u5927');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','62','�����ƵĿ�','\u53d7\u9650\u5236\u7684\u5361\u6216\u78c1\u6761\u4fe1\u606f\u6709\u8bef','\u53d7\u9650\u5236\u7684\u5361\u6216\u78c1\u6761\u4fe1\u606f\u6709\u8bef');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','63','Υ����ȫ���ܹ涨','\u8fdd\u53cd\u5b89\u5168\u4fdd\u5bc6\u89c4\u5b9a','\u8fdd\u53cd\u5b89\u5168\u4fdd\u5bc6\u89c4\u5b9a');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','64','ԭʼ����ȷ','\u539f\u59cb\u91d1\u989d\u4e0d\u6b63\u786e','\u539f\u59cb\u91d1\u989d\u4e0d\u6b63\u786e');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','65','����ȡ���������','\u8d85\u51fa\u53d6\u6b3e\u6b21\u6570\u9650\u5236','\u8d85\u51fa\u53d6\u6b3e\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','66','�ܿ�����������ȫ���ܲ���','\u53d7\u5361\u65b9\u547c\u53d7\u7406\u65b9\u5b89\u5168\u4fdd\u5bc6\u90e8\u95e8','\u53d7\u5361\u65b9\u547c\u53d7\u7406\u65b9\u5b89\u5168\u4fdd\u5bc6\u90e8\u95e8');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','67','��׽��û�տ���','\u6355\u6349\uff08\u6ca1\u6536\u5361\uff09','\u6355\u6349\uff08\u6ca1\u6536\u5361\uff09');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','68','�յ��Ļش�̫��','\u53d1\u5361\u884c\u89c4\u5b9a\u65f6\u95f4\u5185\u6ca1\u6709\u56de\u7b54','\u53d1\u5361\u884c\u89c4\u5b9a\u65f6\u95f4\u5185\u6ca1\u6709\u56de\u7b54');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','75','���������PIN��������','\u5141\u8bb8\u7684\u8f93\u5165PIN\u6b21\u6570\u8d85\u9650','\u5141\u8bb8\u7684\u8f93\u5165PIN\u6b21\u6570\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','77','��Ҫ����������ǩ��','POS\u6279\u6b21\u4e0e\u7f51\u7edc\u4e2d\u5fc3\u4e0d\u4e00\u81f4','POS\u6279\u6b21\u4e0e\u7f51\u7edc\u4e2d\u5fc3\u4e0d\u4e00\u81f4');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','79','�ѻ����׶��˲�ƽ','POS\u7ec8\u7aef\u4e0a\u4f20\u7684\u8131\u673a\u6570\u636e\u5bf9\u8d26\u4e0d\u5e73','POS\u7ec8\u7aef\u4e0a\u4f20\u7684\u8131\u673a\u6570\u636e\u5bf9\u8d26\u4e0d\u5e73');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','90','�����л����ڴ���','\u65e5\u671f\u5207\u6362\u6b63\u5728\u5904\u7406','\u65e5\u671f\u5207\u6362\u6b63\u5728\u5904\u7406');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','91','�����л��������ܲ���','\u7535\u8bdd\u67e5\u8be2\u53d1\u5361\u65b9\u6216\u94f6\u8054\uff0c\u53ef\u91cd\u4f5c','\u7535\u8bdd\u67e5\u8be2\u53d1\u5361\u65b9\u6216\u94f6\u8054\uff0c\u53ef\u91cd\u4f5c');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','92','���ڻ������м�������ʩ�Ҳ������޷��ﵽ','\u7535\u8bdd\u67e5\u8be2\u53d1\u5361\u65b9\u6216\u7f51\u7edc\u4e2d\u5fc3\uff0c\u53ef\u91cd\u4f5c','\u7535\u8bdd\u67e5\u8be2\u53d1\u5361\u65b9\u6216\u7f51\u7edc\u4e2d\u5fc3\uff0c\u53ef\u91cd\u4f5c');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','93','����Υ�����������','\u4ea4\u6613\u8fdd\u6cd5\u3001\u4e0d\u80fd\u5b8c\u6210','\u4ea4\u6613\u8fdd\u6cd5\u3001\u4e0d\u80fd\u5b8c\u6210');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','94','�ظ�����','\u67e5\u8be2\u7f51\u7edc\u4e2d\u5fc3\uff0c\u53ef\u91cd\u65b0\u7b7e\u5230\u4f5c\u4ea4\u6613','\u67e5\u8be2\u7f51\u7edc\u4e2d\u5fc3\uff0c\u53ef\u91cd\u65b0\u7b7e\u5230\u4f5c\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','95','���ڿ��ƴ�','\u8c03\u8282\u63a7\u5236\u9519','\u8c03\u8282\u63a7\u5236\u9519');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','96','ϵͳ�쳣��ʧЧ','\u53d1\u5361\u65b9\u6216\u7f51\u7edc\u4e2d\u5fc3\u51fa\u73b0\u6545\u969c','\u53d1\u5361\u65b9\u6216\u7f51\u7edc\u4e2d\u5fc3\u51fa\u73b0\u6545\u969c');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','97','POS�ն˺��Ҳ���','\u7ec8\u7aef\u672a\u5728\u4e2d\u5fc3\u6216\u94f6\u884c\u767b\u8bb0','\u7ec8\u7aef\u672a\u5728\u4e2d\u5fc3\u6216\u94f6\u884c\u767b\u8bb0');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','98','�����ղ���������Ӧ��','\u94f6\u8054\u6536\u4e0d\u5230\u53d1\u5361\u884c\u5e94\u7b54','\u94f6\u8054\u6536\u4e0d\u5230\u53d1\u5361\u884c\u5e94\u7b54');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','99','PIN��ʽ��','PIN\u683c\u5f0f\u9519\uff0c\u53ef\u91cd\u65b0\u7b7e\u5230\u4f5c\u4ea4\u6613','PIN\u683c\u5f0f\u9519\uff0c\u53ef\u91cd\u65b0\u7b7e\u5230\u4f5c\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','F0','���ѳ��������˻�ʱ�Ҳ���ԭ�ʽ���','\u672a\u80fd\u67e5\u8be2\u5230\u539f\u7b14\u4ea4\u6613','\u672a\u80fd\u67e5\u8be2\u5230\u539f\u7b14\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','F1','��������ϵͳ�������ı�������ʧ��','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','F2','�ڲ��̻����׿������ƣ����̻���֧�ִ����п�','\u8be5\u5546\u6237\u4e0d\u652f\u6301\u6b64\u94f6\u884c\u5361','\u8be5\u5546\u6237\u4e0d\u652f\u6301\u6b64\u94f6\u884c\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','F3','���ܴŵ�����','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','F4','��ѯ�ࣺ��Ҫ��ѯ�Ľ��ײ�����','\u67e5\u8be2\u7684\u4ea4\u6613\u4e0d\u5b58\u5728','\u67e5\u8be2\u7684\u4ea4\u6613\u4e0d\u5b58\u5728');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','F5','��iso8583��ʧ��','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','F6','�̻����׽������,��Ч�Ľ��׽��','\u4ea4\u6613\u91d1\u989d\u53d7\u9650','\u4ea4\u6613\u91d1\u989d\u53d7\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','F7','ת�����������','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','F8','ȡǰ��ϵͳ�Ľ�����ˮ����','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','F9','���ͱ��ĸ�����ϵͳʧ��','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FA','�ý��ײ��������п�����֧��','\u4ea4\u6613\u53d7\u9650','\u4ea4\u6613\u53d7\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FB','��ѯmsgchg_infoʧ��,����δ��ͨ','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FC','��ѯsamcard_infoʧ��,psam��δ�ҵ�','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FD','�������ý��ײ�����','\u4ea4\u6613\u4e0d\u5b58\u5728','\u4ea4\u6613\u4e0d\u5b58\u5728');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FE','���������ƽ���','\u4ea4\u6613\u53d7\u9650','\u4ea4\u6613\u53d7\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FF','�������ý���δ��ͨ','\u4ea4\u6613\u672a\u5f00\u901a','\u4ea4\u6613\u672a\u5f00\u901a');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FG','���ն����ں���������������','\u4ea4\u6613\u53d7\u9650','\u4ea4\u6613\u53d7\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FH','�ն˿������ƣ�δ֪��ǰ���׿���','\u4ea4\u6613\u53d7\u9650','\u4ea4\u6613\u53d7\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FI','�ն˿�������,�����ƵĿ�','\u4ea4\u6613\u53d7\u9650','\u4ea4\u6613\u53d7\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FJ','�ն����ƽ��׽��,���������С���ķ�Χ','\u4ea4\u6613\u53d7\u9650','\u4ea4\u6613\u53d7\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FK','�ն˿����޷��жϣ����н������޶�Ŀ���','\u4ea4\u6613\u53d7\u9650','\u4ea4\u6613\u53d7\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FL','���ն˵�ǰ�Ľ����ܶ�����ܽ��׶������','\u4ea4\u6613\u53d7\u9650','\u4ea4\u6613\u53d7\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FM','�������ݿ���̻�������ʧ��','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FN','�̻������������̻�������ý���','\u4ea4\u6613\u53d7\u9650','\u4ea4\u6613\u53d7\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FO','����pep_jnls����ʧ��','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FP','����EWP_INFOʧ��','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FQ','�ն�����������ʹ��','\u7ec8\u7aef\u6b63\u5e38\uff0c\u53ef\u4ee5\u4f7f\u7528','\u7ec8\u7aef\u6b63\u5e38\uff0c\u53ef\u4ee5\u4f7f\u7528');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FR','�ն������','\u7ec8\u7aef\u5ba1\u6838\u4e2d','\u7ec8\u7aef\u5ba1\u6838\u4e2d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FS','�ն˳�����','\u7ec8\u7aef\u64a4\u673a\u4e2d','\u7ec8\u7aef\u64a4\u673a\u4e2d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FT','�ն�ͣ����','\u7aef\u505c\u673a\u4e2d','\u7aef\u505c\u673a\u4e2d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FU','�ն˸�����','\u7ec8\u7aef\u590d\u673a\u4e2d','\u7ec8\u7aef\u590d\u673a\u4e2d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FV','�ն�δʹ��','\u7ec8\u7aef\u672a\u4f7f\u7528','\u7ec8\u7aef\u672a\u4f7f\u7528');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FW','�ն�ʹ����','\u7ec8\u7aef\u4f7f\u7528\u4e2d','\u7ec8\u7aef\u4f7f\u7528\u4e2d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FX','�ն���ͣ��','\u7ec8\u7aef\u5df2\u505c\u673a','\u7ec8\u7aef\u5df2\u505c\u673a');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FY','��ѯpsam��ǰ��״̬��Ϣʧ��','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','FZ','���ݿ��������','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','H0','����֧��״̬δ֪��5����֮�ڲ������ٴ�֧��','\u4ea4\u6613\u53d7\u9650','\u4ea4\u6613\u53d7\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','E0','ϵͳ���״���','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','EN','���ܻ��������������ܻ�����','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Sl','�̻���Կ������Ҫ��ǩ��','\u5546\u6237\u672a\u7b7e\u5230','\u5546\u6237\u672a\u7b7e\u5230');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','N1','��ʱ','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','N2','����ʧ��','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','A0','POS �ն˶��յ� POS ���ĵ���׼Ӧ����Ϣ,��֤ MAC ����','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','A1','�쳣����','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','A2','������ʱ','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','A3','�����ɹ�','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','A4','�����쳣����','\u51b2\u6b63\u5f02\u5e38\u9519\u8bef','\u51b2\u6b63\u5f02\u5e38\u9519\u8bef');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','C1','��Ч���̻���','\u65e0\u6548\u5546\u6237','\u65e0\u6548\u5546\u6237');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','C2','��Ч���ն˺�','\u65e0\u6548\u7ec8\u7aef','\u65e0\u6548\u7ec8\u7aef');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','C3','�̻��ۿ�·�����ò�����','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','C4','�ý��׽������˻����߳����������ٴγ���','\u8be5\u4ea4\u6613\u91d1\u989d\u5df2\u9000\u8d27\u6216\u8005\u64a4\u9500','\u8be5\u4ea4\u6613\u91d1\u989d\u5df2\u9000\u8d27\u6216\u8005\u64a4\u9500');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','C5','δ���ҵ�ԭ�ʽ���','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','C6','�˻����׽���','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','C7','�˻������������ؽ���쳣','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u54a8\u8be2\u5ba2\u670d\u70ed\u7ebf400-700-8010');

-- add by lp 8/31 16:50
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','H1','����ϵͳ��������ԭ�򣬽�����ʱ�ر�','\u7531\u4e8e\u7cfb\u7edf\u6216\u8005\u5176\u4ed6\u539f\u56e0\uff0c\u4ea4\u6613\u6682\u65f6\u5173\u95ed','\u7531\u4e8e\u7cfb\u7edf\u6216\u8005\u5176\u4ed6\u539f\u56e0\uff0c\u4ea4\u6613\u6682\u65f6\u5173\u95ed');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','H2','�տ���ն�δ��ͨ','\u6536\u6b3e\u4ea4\u6613\u7ec8\u7aef\u672a\u5f00\u901a','\u6536\u6b3e\u4ea4\u6613\u7ec8\u7aef\u672a\u5f00\u901a');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','H3','��ʷ���ײ�ѯû�м�¼','\u5386\u53f2\u4ea4\u6613\u67e5\u8be2\u6ca1\u6709\u8bb0\u5f55','\u5386\u53f2\u4ea4\u6613\u67e5\u8be2\u6ca1\u6709\u8bb0\u5f55');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','H4','ȡPOS�ն����͵�41�����','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','H5','ȡPOS�ն����͵�42�����','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','H6','ȡPOS�ն����͵�60�����','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','H7','���ó����ֶ�ʧ��','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','H8','���ѳ����Ƿ���Ҫ������Ϣ�ͺ�̨��ƥ��','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u4ea4\u6613\u9519\u8bef\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','H9','�ն����͵ı��ĸ�ʽ����','\u7ec8\u7aef\u4e0a\u9001\u7684\u62a5\u6587\u683c\u5f0f\u4e0d\u5bf9','\u7ec8\u7aef\u4e0a\u9001\u7684\u62a5\u6587\u683c\u5f0f\u4e0d\u5bf9');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','HA','�ն�δ���','\u7ec8\u7aef\u672a\u5165\u5e93','\u7ec8\u7aef\u672a\u5165\u5e93');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','HB','ǩ��ʧ��','\u7b7e\u5230\u5931\u8d25','\u7b7e\u5230\u5931\u8d25');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','HC','����ˢ��������ʱ������','\u4e0d\u662f\u5237\u5361\u4ea4\u6613\u6682\u65f6\u4e0d\u53d7\u7406','\u4e0d\u662f\u5237\u5361\u4ea4\u6613\u6682\u65f6\u4e0d\u53d7\u7406');


insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','T1','��������ʱ�����κŲ�һ�²���û������������','\u53c2\u6570\u4e0b\u8f7d\u5931\u8d25\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u53c2\u6570\u4e0b\u8f7d\u5931\u8d25\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','T3','��������ʱ�����κ�һ�£�����û������������','\u53c2\u6570\u4e0b\u8f7d\u5931\u8d25\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u53c2\u6570\u4e0b\u8f7d\u5931\u8d25\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','T4','������ʱ�����͵����κŲ�һ��','\u7ed3\u7b97\u5931\u8d25\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458','\u7ed3\u7b97\u5931\u8d25\uff0c\u8bf7\u8054\u7cfb\u5ba2\u670d\u4eba\u5458');


-- �����Ӻ���ϵͳ��صĽ���Ӧ���� 
-- he.qingqi
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','V1','���ƿ�Bin���н���','\u9650\u5236\u5361Bin\u8fdb\u884c\u4ea4\u6613','\u9650\u5236\u5361Bin\u8fdb\u884c\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','V2','���ܵ��ڿ�Bin���Ƶ���ͽ��','\u4e0d\u80fd\u4f4e\u4e8e\u5361Bin\u9650\u5236\u7684\u6700\u4f4e\u91d1\u989d','\u4e0d\u80fd\u4f4e\u4e8e\u5361Bin\u9650\u5236\u7684\u6700\u4f4e\u91d1\u989d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','V3','���ܵ��ڿ�Bin���Ƶ���߽��','\u4e0d\u80fd\u4f4e\u4e8e\u5361Bin\u9650\u5236\u7684\u6700\u9ad8\u91d1\u989d','\u4e0d\u80fd\u4f4e\u4e8e\u5361Bin\u9650\u5236\u7684\u6700\u9ad8\u91d1\u989d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','V4','���ܵ��ڿ�Bin���Ƶ��ս�����ͽ��','\u4e0d\u80fd\u4f4e\u4e8e\u5361Bin\u9650\u5236\u7684\u65e5\u4ea4\u6613\u6700\u4f4e\u91d1\u989d','\u4e0d\u80fd\u4f4e\u4e8e\u5361Bin\u9650\u5236\u7684\u65e5\u4ea4\u6613\u6700\u4f4e\u91d1\u989d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','V5','���ܵ��ڿ�Bin���Ƶ��ս�����߽��','\u4e0d\u80fd\u4f4e\u4e8e\u5361Bin\u9650\u5236\u7684\u65e5\u4ea4\u6613\u6700\u9ad8\u91d1\u989d','\u4e0d\u80fd\u4f4e\u4e8e\u5361Bin\u9650\u5236\u7684\u65e5\u4ea4\u6613\u6700\u9ad8\u91d1\u989d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','W1','�����п������������Ʋ��ܽ���','\u8be5\u94f6\u884c\u5361\u9ed1\u540d\u5355\u88ab\u9650\u5236\u4e0d\u80fd\u4ea4\u6613','\u8be5\u94f6\u884c\u5361\u9ed1\u540d\u5355\u88ab\u9650\u5236\u4e0d\u80fd\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','W2','�����п����������ʽ��׽���','\u8be5\u94f6\u884c\u5361\u9ed1\u540d\u5355\u5355\u7b14\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u94f6\u884c\u5361\u9ed1\u540d\u5355\u5355\u7b14\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','W3','�����п��������ս��׽���','\u8be5\u94f6\u884c\u5361\u9ed1\u540d\u5355\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u94f6\u884c\u5361\u9ed1\u540d\u5355\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','W4','���Ƹ����п����н���','\u9650\u5236\u8be5\u94f6\u884c\u5361\u8fdb\u884c\u4ea4\u6613','\u9650\u5236\u8be5\u94f6\u884c\u5361\u8fdb\u884c\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','W5','���ܵ��ڸ����п���ͽ��','\u4e0d\u80fd\u4f4e\u4e8e\u8be5\u94f6\u884c\u5361\u6700\u4f4e\u91d1\u989d','\u4e0d\u80fd\u4f4e\u4e8e\u8be5\u94f6\u884c\u5361\u6700\u4f4e\u91d1\u989d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','W6','���ܵ��ڸ����п���߽��','\u4e0d\u80fd\u4f4e\u4e8e\u8be5\u94f6\u884c\u5361\u6700\u9ad8\u91d1\u989d','\u4e0d\u80fd\u4f4e\u4e8e\u8be5\u94f6\u884c\u5361\u6700\u9ad8\u91d1\u989d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','W7','���ܵ��ڸ����п��ս�����ͽ��','\u4e0d\u80fd\u4f4e\u4e8e\u8be5\u94f6\u884c\u5361\u65e5\u4ea4\u6613\u6700\u4f4e\u91d1\u989d','\u4e0d\u80fd\u4f4e\u4e8e\u8be5\u94f6\u884c\u5361\u65e5\u4ea4\u6613\u6700\u4f4e\u91d1\u989d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','W8','���ܵ��ڸ����п��ս�����߽��','\u4e0d\u80fd\u4f4e\u4e8e\u8be5\u94f6\u884c\u5361\u65e5\u4ea4\u6613\u6700\u9ad8\u91d1\u989d','\u4e0d\u80fd\u4f4e\u4e8e\u8be5\u94f6\u884c\u5361\u65e5\u4ea4\u6613\u6700\u9ad8\u91d1\u989d');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Y1','���ն˺��������ܽ���','\u8be5\u7ec8\u7aef\u9ed1\u540d\u5355\u4e0d\u80fd\u4ea4\u6613','\u8be5\u7ec8\u7aef\u9ed1\u540d\u5355\u4e0d\u80fd\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Y2','���ն˺���������ʹ�����ÿ�����','\u8be5\u7ec8\u7aef\u9ed1\u540d\u5355\u4e0d\u80fd\u4f7f\u7528\u4fe1\u7528\u5361\u4ea4\u6613','\u8be5\u7ec8\u7aef\u9ed1\u540d\u5355\u4e0d\u80fd\u4f7f\u7528\u4fe1\u7528\u5361\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Y3','���ն˺������µ��ʽ��׽���','\u8be5\u7ec8\u7aef\u9ed1\u540d\u5355\u4e0b\u5355\u7b14\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u7ec8\u7aef\u9ed1\u540d\u5355\u4e0b\u5355\u7b14\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Y4','���ն˺��������ս��׽���','\u8be5\u7ec8\u7aef\u9ed1\u540d\u5355\u4e0b\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u7ec8\u7aef\u9ed1\u540d\u5355\u4e0b\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Y5','���ն˲�������н���','\u8be5\u7ec8\u7aef\u4e0d\u5141\u8bb8\u8fdb\u884c\u4ea4\u6613','\u8be5\u7ec8\u7aef\u4e0d\u5141\u8bb8\u8fdb\u884c\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Y6','���ն������ÿ�����ʹ��','\u8be5\u7ec8\u7aef\u4e0b\u4fe1\u7528\u5361\u4e0d\u80fd\u4f7f\u7528','\u8be5\u7ec8\u7aef\u4e0b\u4fe1\u7528\u5361\u4e0d\u80fd\u4f7f\u7528');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Y7','���ն��½�ǿ�����ʹ��','\u8be5\u7ec8\u7aef\u4e0b\u501f\u8bb0\u5361\u4e0d\u80fd\u4f7f\u7528','\u8be5\u7ec8\u7aef\u4e0b\u501f\u8bb0\u5361\u4e0d\u80fd\u4f7f\u7528');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Y8','���ն������ÿ����ʽ���','\u8be5\u7ec8\u7aef\u4e0b\u4fe1\u7528\u5361\u5355\u7b14\u91d1\u989d\u8d85\u9650','\u8be5\u7ec8\u7aef\u4e0b\u4fe1\u7528\u5361\u5355\u7b14\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Y9','���ն��½�ǿ����ʽ���','\u8be5\u7ec8\u7aef\u4e0b\u501f\u8bb0\u5361\u5355\u7b14\u91d1\u989d\u8d85\u9650','\u8be5\u7ec8\u7aef\u4e0b\u501f\u8bb0\u5361\u5355\u7b14\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','YA','���ն������ÿ��ս��׽���','\u8be5\u7ec8\u7aef\u4e0b\u4fe1\u7528\u5361\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u7ec8\u7aef\u4e0b\u4fe1\u7528\u5361\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','YB','���ն��½�ǿ��ս��׽���','\u8be5\u7ec8\u7aef\u4e0b\u501f\u8bb0\u5361\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u7ec8\u7aef\u4e0b\u501f\u8bb0\u5361\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','X1','���̻����ܳ���','\u8be5\u5546\u6237\u4e0d\u80fd\u64a4\u9500','\u8be5\u5546\u6237\u4e0d\u80fd\u64a4\u9500');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','X2','���̻������˻�','\u8be5\u5546\u6237\u4e0d\u80fd\u9000\u8d27','\u8be5\u5546\u6237\u4e0d\u80fd\u9000\u8d27');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','X3','���̻����������ܽ���','\u8be5\u5546\u6237\u9ed1\u540d\u5355\u4e0d\u80fd\u4ea4\u6613','\u8be5\u5546\u6237\u9ed1\u540d\u5355\u4e0d\u80fd\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','X4','���̻�����������ʹ�����ÿ�����','\u8be5\u5546\u6237\u9ed1\u540d\u5355\u4e0d\u80fd\u4f7f\u7528\u4fe1\u7528\u5361\u4ea4\u6613','\u8be5\u5546\u6237\u9ed1\u540d\u5355\u4e0d\u80fd\u4f7f\u7528\u4fe1\u7528\u5361\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','X5','���̻��������µ��ʽ��׽���','\u8be5\u5546\u6237\u9ed1\u540d\u5355\u4e0b\u5355\u7b14\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u5546\u6237\u9ed1\u540d\u5355\u4e0b\u5355\u7b14\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','X6','���̻����������ս��׽���','\u8be5\u5546\u6237\u9ed1\u540d\u5355\u4e0b\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u5546\u6237\u9ed1\u540d\u5355\u4e0b\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','X7','���̻����ܽ��н���','\u8be5\u5546\u6237\u4e0d\u80fd\u8fdb\u884c\u4ea4\u6613','\u8be5\u5546\u6237\u4e0d\u80fd\u8fdb\u884c\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','X8','���̻����������ÿ�����','\u8be5\u5546\u6237\u4e0d\u5141\u8bb8\u4fe1\u7528\u5361\u4ea4\u6613','\u8be5\u5546\u6237\u4e0d\u5141\u8bb8\u4fe1\u7528\u5361\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','X9','���̻��������ǿ�����','\u8be5\u5546\u6237\u4e0d\u5141\u8bb8\u501f\u8bb0\u5361\u4ea4\u6613','\u8be5\u5546\u6237\u4e0d\u5141\u8bb8\u501f\u8bb0\u5361\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XA','���̻������ÿ����ʽ��׽���','\u8be5\u5546\u6237\u4e0b\u4fe1\u7528\u5361\u5355\u7b14\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u5546\u6237\u4e0b\u4fe1\u7528\u5361\u5355\u7b14\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XB','���̻��½�ǿ����ʽ��׽���','\u8be5\u5546\u6237\u4e0b\u501f\u8bb0\u5361\u5355\u7b14\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u5546\u6237\u4e0b\u501f\u8bb0\u5361\u5355\u7b14\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XC','���̻������ÿ��ս��׽���','\u8be5\u5546\u6237\u4e0b\u4fe1\u7528\u5361\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u5546\u6237\u4e0b\u4fe1\u7528\u5361\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XD','���̻��½�ǿ��ս��׽���','\u8be5\u5546\u6237\u4e0b\u501f\u8bb0\u5361\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u5546\u6237\u4e0b\u501f\u8bb0\u5361\u65e5\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Z1','�������²��ܽ��н���','\u8be5\u6e20\u9053\u4e0b\u4e0d\u80fd\u8fdb\u884c\u4ea4\u6613','\u8be5\u6e20\u9053\u4e0b\u4e0d\u80fd\u8fdb\u884c\u4ea4\u6613');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Z2','�������²���ʹ�����ÿ�','\u8be5\u6e20\u9053\u4e0b\u4e0d\u80fd\u4f7f\u7528\u4fe1\u7528\u5361','\u8be5\u6e20\u9053\u4e0b\u4e0d\u80fd\u4f7f\u7528\u4fe1\u7528\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Z3','�������²���ʹ�ý�ǿ�','\u8be5\u6e20\u9053\u4e0b\u4e0d\u80fd\u4f7f\u7528\u501f\u8bb0\u5361','\u8be5\u6e20\u9053\u4e0b\u4e0d\u80fd\u4f7f\u7528\u501f\u8bb0\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Z4','�����������ÿ���ͽ��׽���','\u8be5\u6e20\u9053\u4e0b\u4fe1\u7528\u5361\u6700\u4f4e\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u6e20\u9053\u4e0b\u4fe1\u7528\u5361\u6700\u4f4e\u4ea4\u6613\u91d1\u989d\u8d85\u9650');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Z5','�������½�ǿ���ͽ��׽���','\u8be5\u6e20\u9053\u4e0b\u501f\u8bb0\u5361\u6700\u4f4e\u4ea4\u6613\u91d1\u989d\u8d85\u9650','\u8be5\u6e20\u9053\u4e0b\u501f\u8bb0\u5361\u6700\u4f4e\u4ea4\u6613\u91d1\u989d\u8d85\u9650');

-- add by lp 
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','A6','ת�˻�������ʱ,��ȷ���Ƿ���ɹ�','\u4ea4\u6613\u8d85\u65f6','\u4ea4\u6613\u8d85\u65f6');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','V6','���̻��� ���ܸ��ڿ�Bin���Ƶ���߽��','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','V7','���ն��� ���ƿ�Bin���н���','\u53d7\u9650\u5236\u7684\u5361','\u53d7\u9650\u5236\u7684\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','V8','���ն��� ���ܵ��ڿ�Bin���Ƶ���ͽ��','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','V9','���ն��� ���ܸ��ڿ�Bin���Ƶ���߽��','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','W9','�����п�����֧������������','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','YC','���ն˺������²����жϸ����п��Ŀ���','\u672a\u77e5\u5361','\u672a\u77e5\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','YE','���ն��²����жϸ����п��Ŀ���','\u672a\u77e5\u5361','\u672a\u77e5\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','YF','���ն��½�ǿ�����֧���Ĵ�������','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','YG','���ն������ÿ�����֧���Ĵ�������','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','YH','���ն���ͬһ��ǿ�����֧����������','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','YI','���ն���ͬһ���ÿ�����֧����������','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','YJ','���ն��¶����п���֧����������','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','YK','���ն��¶��������п���֧����������','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XE','���̻��������²����жϸ����п��Ŀ���','\u672a\u77e5\u5361','\u672a\u77e5\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XF','���̻��²����жϸ����п��Ŀ���','\u672a\u77e5\u5361','\u672a\u77e5\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XG','���̻��½�ǿ�����֧����������','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XH','���̻������ÿ�����֧����������','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XI','���̻���ͬһ��������Ž���֧����������','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XJ','���̻���ͬһ���ÿ����Ž���֧����������','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XK','���̻��¶����п�����֧����������','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','XL','���̻��¶��������п�����֧����������','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Z6','�������²����жϸ����п��Ŀ���','\u672a\u77e5\u5361','\u672a\u77e5\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Z7','�����������ÿ��ս��׽���','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','Z8','�������½�ǿ��ս��׽���','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','P1','���̻��� �������п����н���','\u53d7\u9650\u5236\u7684\u5361','\u53d7\u9650\u5236\u7684\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','P2','���̻��� �������п����״�������','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','P3','���̻��� ���ܵ��ڸ����п�����ͽ��','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','P4','���̻��� ���ܸ��ڸ����п�����߽��','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','P5','���̻��� ���ܵ��ڸ����п��ս������','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','P6','���̻��� ���ܸ��ڸ����п��ս�����߽��','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','P7','���̻��� �����п����������ܽ���','\u4ea4\u6613\u9650\u5236','\u4ea4\u6613\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','P8','���̻��� �����п����������ʽ���','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','P9','���̻��� �����п��������ս��׽���','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','PA','���ն��� �����п����������ܽ���','\u4ea4\u6613\u9650\u5236','\u4ea4\u6613\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','PB','���ն��� �����п����������ʽ���','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','PC','���ն��� �����п��������ս��׽���','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','PE','���ն��� �������п����н���','\u53d7\u9650\u5236\u7684\u5361','\u53d7\u9650\u5236\u7684\u5361');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','PF','���ն��� �������п����״�������','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','PG','���ն��� ���ܵ��ڸ����п�����ͽ��','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','PH','���ն��� ���ܸ��ڸ����п�����߽��','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','PI','���ն��� ���ܵ��ڸ����п��ս�����ͽ��','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','PJ','���ն��� ���ܸ��ڸ����п��ս�����߽��','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','G1','����ת�˻�� ��ת�������ʽ���','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','G2','����ת�˻�� ��ת�����ս��׽���','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','G3','����ת�˻�� ��ת�������״�������','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','G4','ATMת�˻�� ��ת�������ʽ���','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','G5','ATMת�˻�� ��ת�����ս��׽���','\u4ea4\u6613\u91d1\u989d\u9650\u5236','\u4ea4\u6613\u91d1\u989d\u9650\u5236');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','G6','ATMת�˻�� ��ת�������״�������','\u4ea4\u6613\u6b21\u6570\u9650\u5236','\u4ea4\u6613\u6b21\u6570\u9650\u5236');

-- lp by leep
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('FFF','G7','����ת�˻�� �ý������·�ɲ�����','\u91d1\u989d\u4e0d\u5408\u6cd5','\u91d1\u989d\u4e0d\u5408\u6cd5');

-- ����������ת�˻�����⽻��Ӧ����
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('208','A6','ת�˻�ʱ,����ͷ�ȷ�Ͻ��ס�','\u8f6c\u8d26\u6c47\u6b3e\u8d85\u65f6,\u8bf7\u5411\u5ba2\u670d\u786e\u8ba4\u4ea4\u6613\u3002','\u8f6c\u8d26\u6c47\u6b3e\u8d85\u65f6,\u8bf7\u5411\u5ba2\u670d\u786e\u8ba4\u4ea4\u6613\u3002');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('208','59','��Ƭ����ʧ�ܡ�','\u5361\u7247\u68c0\u9a8c\u5931\u8d25\u3002','\u5361\u7247\u68c0\u9a8c\u5931\u8d25\u3002');
insert into xpep_retcode(xpep_trancde,xpep_retcode,xpep_content,xpep_content_ewp,xpep_content_ewp_print) values('208','36','ת���������ơ�','\u8f6c\u51fa\u5361\u53d7\u9650\u5236\u3002','\u8f6c\u51fa\u5361\u53d7\u9650\u5236\u3002');