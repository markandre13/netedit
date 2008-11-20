/*
 * Create PostgreSQL Tables for NetEdit
 * Written 2005, 2006 by Mark-André Hopf <mhopf@mark13.org>
 *
 * Install with:
 *   createdb netedit
 *   psql -f create.sql netedit
 *
 * Notes:
 * PostgreSQL Embedded SQL doesn't like the table name 'connection' so we
 * use 'conn' instead.
 */

DROP TABLE usergroup;
DROP TABLE users;
DROP TABLE objectgroup;
DROP TABLE rights;
DROP TABLE groups;

DROP TABLE conn;
DROP TABLE symbol;
DROP TABLE map;
DROP TABLE icon;
DROP TABLE figure;

DROP TABLE coupledwith;
DROP TABLE network;
DROP TABLE interface;
DROP TABLE node;
DROP TABLE iftypename;

DROP TABLE uniqueid;

DROP TABLE trapdlog;

CREATE TABLE uniqueid (
  id              bigint,
  PRIMARY KEY(id)
);

/*
 *
 * Topology Tables 
 *
 */

/**
 * A computing device like a server, gateway, firewall, switch, ...
 */
CREATE TABLE node (
  node_id         bigint,

  sysObjectID     varchar(251), /* 'what kind of box' being managed */
  sysName         varchar(255),
  sysContact      varchar(255),
  sysLocation     varchar(255),
  sysDescr        varchar(255),

  ipforwarding    boolean,
  
  mgmtaddr        inet,         /* varchar(39) for non-pgsql */
  
  mgmtflags       integer,      /* 1: unknown
                                   2: SNMP
                                   4: ping
                                   8: telnet
                                  16: ssh
                                  32: http
                                  64: https
                                8192: mGuard/G.A.I */
  topoflags       integer,      /* 1: unknown
                                   2: removed from toplogy
                                  16: connector
                                  32: gateway
                                  64: star type hub
                                 128: bridge
                                 256: SNMP capable connector */
  status          integer,      /* 1: unknown
                                   2: normal or up
                                   3: marginal
                                   4: critical or down
                                   5: unmanaged
                                   6: acknowledge */

  ctime           timestamp,    /* time this entry was created */
  cuser           varchar(80),  /* user who created this node */
  mtime           timestamp,    /* time this entry was modified */
  muser           varchar(80),  /* last user who modified this entry */
  stime           timestamp,    /* last time a related symbol was modified */
  suser           varchar(80),  /* user who modified the related symbol */

  updated         timestamp,    /* last time of successfull query */

  PRIMARY KEY(node_id),
  FOREIGN KEY(node_id) REFERENCES uniqueid(id)
);

CREATE TABLE interface (
  interface_id    bigint,
  node_id         bigint,
  status          integer,      /* as in 'node' */
  flags           integer,      /* 0: undefined
                                   1: removed from topology
                                   2: user added to interface
                                  16: bus type interface
                                  32: star type interface 
                                  48: token-ring interface
                                  64: FDDI interface
                                  80: serial interface
                                 256: is main hub of a star segment
                                 512: not connected to segment
                                1024: not connected to net
                                1536: not connected to segment and net
                                8192: do not ping this interface
                               16383: in port position on star hub
                               32768: in absolute location on segment */
  ipaddress       inet,         /* ip address and network mask */
  
  ifIndex         integer,
  ifDescr         varchar(255),
  ifType          integer,      /* see table 'iftypename' for full names */
  ifPhysAddress   varchar(36),

  PRIMARY KEY(interface_id),
  FOREIGN KEY(node_id) REFERENCES node(node_id),
  FOREIGN KEY(interface_id) REFERENCES uniqueid(id)
);

CREATE TABLE network (
  network_id      bigint,
  name            varchar(255),
  parent          bigint,
  issegment       boolean,
  flags           integer,      /* 0: undefined  
                                   1: removed from topology
                                   2: user added to network
                                  16: serial network ?
                                  32: network ping */
  status          integer,      /* 1: unknown
                                   2: normal or up
                                   3: marginal
                                   4: critical or down    
                                   5: unmanaged
                                   6: acknowledge */
  network         cidr,
  defaultseg      bigint,

  PRIMARY KEY (network_id),
  FOREIGN KEY(network_id) REFERENCES uniqueid(id)
);

CREATE TABLE coupledwith (
  interface_id    bigint,      /* interface id */
  network_id      bigint,      /* segment or network id */

  FOREIGN KEY (interface_id) REFERENCES interface(interface_id),
  FOREIGN KEY (network_id) REFERENCES network(network_id)
);

/*
 *
 * User Interface Tables
 *
 */

/*
 * A map
 */
CREATE TABLE map (
  map_id          bigint,
  name            varchar(255),
  comment         varchar(255),
  represents	  bigint,
  /*
    0: No layout
    1: Row/Column
    2: Point to Point
    3: Bus
    4: Star
    5: Ring
    6: Tree
  */
  layout          integer,
  PRIMARY KEY(map_id),
  FOREIGN KEY(map_id) REFERENCES uniqueid(id)
);

CREATE TABLE symbol (
  symbol_id       bigint,
  map_id          bigint,
  id		  bigint,
  xpos            integer,
  ypos            integer,
  PRIMARY KEY (symbol_id, map_id),
  FOREIGN KEY (map_id) REFERENCES map(map_id),
  FOREIGN KEY (id) REFERENCES uniqueid(id)
);

CREATE TABLE conn (
  conn_id	  bigint,
  map_id          bigint,
  id0             bigint,  /* symbol id */
  id1             bigint,  /* symbol id */
  label           varchar(80),
  type            integer,
  status          integer,
  PRIMARY KEY (conn_id, map_id),
  FOREIGN KEY (id0, map_id) REFERENCES symbol(symbol_id, map_id),
  FOREIGN KEY (id1, map_id) REFERENCES symbol(symbol_id, map_id),
  FOREIGN KEY (map_id) REFERENCES map(map_id)
);

CREATE TABLE icon (
  sysObjectID     varchar(251), /* icon is a default for this node type */
  name            varchar(251), /* name, ie. 'Computer:PC' */
  figure          text,         /* figure in TOAD ATV format */
  PRIMARY KEY (sysObjectID)
);

CREATE TABLE figure (
  figure_id       bigint,
  figure          text,
  PRIMARY KEY (figure_id),
  FOREIGN KEY (figure_id) REFERENCES uniqueid(id)
);

CREATE TABLE iftypename (
  ifType          integer,
  name            varchar(80),
  PRIMARY KEY (ifType)
);
INSERT INTO iftypename VALUES ( 1, 'other');
INSERT INTO iftypename VALUES ( 2, 'regular1822');
INSERT INTO iftypename VALUES ( 3, 'hdh1822');
INSERT INTO iftypename VALUES ( 4, 'ddn-x25');
INSERT INTO iftypename VALUES ( 5, 'rfc877');
INSERT INTO iftypename VALUES ( 6, 'ethernet-csmacd');
INSERT INTO iftypename VALUES ( 7, 'iso88023-csmacd');
INSERT INTO iftypename VALUES ( 8, 'iso88024-tokenBus');
INSERT INTO iftypename VALUES ( 9, 'iso88025-tokenRing');
INSERT INTO iftypename VALUES (10, 'iso88026-man');
INSERT INTO iftypename VALUES (11, 'starLan');
INSERT INTO iftypename VALUES (12, 'proteon-10Mbit');
INSERT INTO iftypename VALUES (13, 'proteon-80Mbit');
INSERT INTO iftypename VALUES (14, 'hyperchannel');
INSERT INTO iftypename VALUES (15, 'fddi');
INSERT INTO iftypename VALUES (16, 'lapb');
INSERT INTO iftypename VALUES (17, 'sdlc');
INSERT INTO iftypename VALUES (18, 'tl-carrier');
INSERT INTO iftypename VALUES (19, 'cept');
INSERT INTO iftypename VALUES (20, 'basicISDN');
INSERT INTO iftypename VALUES (21, 'primaryISDN');
INSERT INTO iftypename VALUES (22, 'propPointToPointSerial');
INSERT INTO iftypename VALUES (23, 'ppp');
INSERT INTO iftypename VALUES (24, 'softwareLoopback');
INSERT INTO iftypename VALUES (25, 'eon');
INSERT INTO iftypename VALUES (26, 'ethernet-3Mbit');
INSERT INTO iftypename VALUES (27, 'nsip');
INSERT INTO iftypename VALUES (28, 'slip');

/*
 *
 * User/Group Management
 *
 */

CREATE TABLE users (
  login           varchar(40),
  password        varchar(40),
  name	          varchar(40),
  email           varchar(40),
  company         varchar(40),
  position        varchar(40),
  phone           varchar(20),
  room            varchar(20),
  city            varchar(40),
  street          varchar(40),
  zipcode         varchar(40),

  PRIMARY KEY (login)
);

CREATE TABLE groups (
  name            varchar(40),
  description     varchar(251),

  ctime           timestamp,
  cuser           varchar(80),
  mtime           timestamp,
  muser           varchar(80),

  PRIMARY KEY (name)
);

/* which users a part of a group */
CREATE TABLE usergroup (
  login             varchar(40),
  group_name        varchar(40),
  FOREIGN KEY (login) REFERENCES users(login),
  FOREIGN KEY (group_name) REFERENCES groups(name)
);

/* which objects (map, node, ...) are part of a group */
CREATE TABLE objectgroup (
  group_name      varchar(40),
  objid           bigint,

  FOREIGN KEY (group_name) REFERENCES groups(name)
);

CREATE TABLE rights (
  group_name	  varchar(40),
  aright          varchar(40),
  FOREIGN KEY (group_name) REFERENCES groups(name)
);

/*
 *
 * Log Tables
 *
 */

CREATE TABLE trapdlog (
  epochtime       integer,    /* UNIX time */
  trap_category	  integer,    /* 0: threshold
                                 1: network topology
                                 2: error
                                 3: status
                                 4: node configuration
                                 5: application alert
                                 6: display in all categories
                                 7: logged, but not displayed
                                 8: map events, not display or logged */
  trap_create_time timestamp,
  ip_hostname      varchar(251),
  trap_source      char(1)    /* a: application
                                 d: demo or loadhosts
                                 ... */
);

/*
 *
 * Informational tables
 *
 */


/*
 *
 * Example Entries
 *
 */
INSERT INTO uniqueid VALUES (1);
INSERT INTO uniqueid VALUES (2);
INSERT INTO uniqueid VALUES (3);
INSERT INTO uniqueid VALUES (4);
INSERT INTO uniqueid VALUES (5);
INSERT INTO uniqueid VALUES (6);
INSERT INTO uniqueid VALUES (7);
INSERT INTO uniqueid VALUES (8);
INSERT INTO uniqueid VALUES (9);
INSERT INTO uniqueid VALUES (10);
INSERT INTO uniqueid VALUES (11);
INSERT INTO uniqueid VALUES (12);

INSERT INTO map(map_id, name) VALUES (1, 'First map');
INSERT INTO map(map_id, name) VALUES (2, 'Second map');
INSERT INTO map(map_id, name) VALUES (3, 'Third map');

INSERT INTO node(node_id, sysObjectID, sysName) VALUES (4, 'mguard', '127.0.0.1');
INSERT INTO node(node_id, sysObjectID, sysName) VALUES (5, 'mguard', '10.1.0.151');
INSERT INTO interface(interface_id, node_id, ipaddress, ifIndex, ifDescr, ifType, ifPhysAddress)
  VALUES( 9, 5, '10.1.0.151', 1, 'en0', 6, '00:0d:93:4f:f9:26');
INSERT INTO interface(interface_id, node_id, ipaddress, ifIndex, ifDescr, ifType, ifPhysAddress)
  VALUES(10, 5, '172.154.12.32', 1, 'en1', 6, '00:0d:93:4f:f9:27');
INSERT INTO node(node_id, sysObjectID, sysName) VALUES (6, 'mac', '10.1.0.152');
INSERT INTO node(node_id, sysObjectID, sysName) VALUES (7, 'mguard', '10.1.0.153');
INSERT INTO node(node_id, sysObjectID, sysName) VALUES (8, 'router', '10.1.0.254');

INSERT INTO icon(sysObjectID, name) VALUES('mac', 'Computer:Mac');
INSERT INTO icon(sysObjectID, name) VALUES('bridge', 'Connector:Bridge');
INSERT INTO icon(sysObjectID, name) VALUES('router', 'Connector:Router');
INSERT INTO icon(sysObjectID, name) VALUES('snpx hub 5000', 'Connector:SNPX HUB 5000');
INSERT INTO icon(sysObjectID, name) VALUES('switch', 'Connector:Switch');
INSERT INTO icon(sysObjectID, name) VALUES('hub', 'Connector:Generic Hub');
INSERT INTO icon(sysObjectID, name) VALUES('map', 'Map:Submap');
INSERT INTO icon(sysObjectID, name) VALUES('mguard', 'Connector:mGuard');

INSERT INTO symbol VALUES (0, 1, 2, 250,  50);
INSERT INTO symbol VALUES (1, 1, 5,  25,  50);
INSERT INTO symbol VALUES (2, 1, 6, 100,  50);
INSERT INTO symbol VALUES (3, 1, 7, 175,  50);
INSERT INTO symbol VALUES (4, 1, 8, 100, 125);

INSERT INTO conn VALUES (1, 1, 1, 4);
INSERT INTO conn VALUES (2, 1, 2, 4);
INSERT INTO conn VALUES (3, 1, 3, 4);

INSERT INTO symbol VALUES (1, 2, 1, 200, 150);
INSERT INTO symbol VALUES (2, 2, 8, 300, 150);

INSERT INTO conn VALUES (1, 2, 1, 2);

INSERT INTO users(login, name, email) VALUES
  ('mark', 'Mark-André Hopf', 'mhopf@mark13.org');

COMMIT;
