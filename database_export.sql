-- MySQL dump 10.13  Distrib 8.0.45, for Win64 (x86_64)
--
-- Host: localhost    Database: helbreath
-- ------------------------------------------------------
-- Server version	8.0.45

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!50503 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `account_database`
--

DROP TABLE IF EXISTS `account_database`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `account_database` (
  `name` varchar(10) NOT NULL DEFAULT '',
  `password` varchar(10) NOT NULL DEFAULT '1',
  `WorldName` varchar(10) NOT NULL DEFAULT 'WS1',
  `AccountID` int unsigned NOT NULL AUTO_INCREMENT,
  `LoginIpAddress` varchar(20) NOT NULL DEFAULT '',
  `IsGMAccount` smallint NOT NULL DEFAULT '0',
  `cash` int unsigned NOT NULL DEFAULT '0',
  `ValidDate` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
  `LoginDate` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
  `BlockDate` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
  `Email` varchar(50) NOT NULL DEFAULT '',
  `Quiz` varchar(45) NOT NULL DEFAULT '',
  `Answer` varchar(20) NOT NULL DEFAULT '',
  `FirstName` varchar(20) NOT NULL DEFAULT '',
  `LastName` varchar(20) NOT NULL DEFAULT '',
  `Address` varchar(50) NOT NULL DEFAULT '',
  `City` varchar(20) NOT NULL DEFAULT '',
  `State` varchar(30) NOT NULL DEFAULT '',
  `ZipCode` varchar(10) NOT NULL DEFAULT '',
  `Country` varchar(20) NOT NULL DEFAULT '',
  PRIMARY KEY (`name`),
  UNIQUE KEY `AccountID` (`AccountID`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=latin1 PACK_KEYS=0;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `account_database`
--

LOCK TABLES `account_database` WRITE;
/*!40000 ALTER TABLE `account_database` DISABLE KEYS */;
INSERT INTO `account_database` VALUES ('CamB','password','WS1',1,'172.16.0.1',0,0,'2026-03-17 17:40:05','0000-00-00 00:00:00','0000-00-00 00:00:00','yes','yes','yes','','','','','','','');
/*!40000 ALTER TABLE `account_database` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `bank_item`
--

DROP TABLE IF EXISTS `bank_item`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `bank_item` (
  `CharID` int unsigned NOT NULL DEFAULT '0',
  `ItemName` varchar(20) NOT NULL DEFAULT '',
  `ItemID` bigint unsigned NOT NULL AUTO_INCREMENT,
  `Count` int unsigned NOT NULL DEFAULT '0',
  `ItemType` tinyint unsigned NOT NULL DEFAULT '0',
  `ID1` smallint NOT NULL DEFAULT '0',
  `ID2` smallint NOT NULL DEFAULT '0',
  `ID3` smallint NOT NULL DEFAULT '0',
  `Color` tinyint unsigned NOT NULL DEFAULT '0',
  `Effect1` int DEFAULT '0',
  `Effect2` int DEFAULT '0',
  `Effect3` int DEFAULT '0',
  `LifeSpan` int unsigned NOT NULL DEFAULT '0',
  `Attribute` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`ItemID`),
  UNIQUE KEY `ItemID` (`ItemID`),
  KEY `Index` (`CharID`),
  KEY `ItemName` (`ItemName`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 PACK_KEYS=0;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `bank_item`
--

LOCK TABLES `bank_item` WRITE;
/*!40000 ALTER TABLE `bank_item` DISABLE KEYS */;
/*!40000 ALTER TABLE `bank_item` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `cash_transactions`
--

DROP TABLE IF EXISTS `cash_transactions`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `cash_transactions` (
  `account_name` varchar(10) NOT NULL,
  `service` varchar(50) NOT NULL,
  `date` datetime NOT NULL DEFAULT '0000-00-00 00:00:00'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `cash_transactions`
--

LOCK TABLES `cash_transactions` WRITE;
/*!40000 ALTER TABLE `cash_transactions` DISABLE KEYS */;
/*!40000 ALTER TABLE `cash_transactions` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `char_database`
--

DROP TABLE IF EXISTS `char_database`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `char_database` (
  `account_name` varchar(10) NOT NULL DEFAULT '',
  `char_name` varchar(10) NOT NULL DEFAULT '',
  `CharID` int unsigned NOT NULL AUTO_INCREMENT,
  `ID1` mediumint NOT NULL DEFAULT '0',
  `ID2` mediumint NOT NULL DEFAULT '0',
  `ID3` mediumint NOT NULL DEFAULT '0',
  `Level` smallint unsigned NOT NULL DEFAULT '1',
  `Strenght` tinyint unsigned NOT NULL DEFAULT '10',
  `Vitality` tinyint unsigned NOT NULL DEFAULT '10',
  `Dexterity` tinyint unsigned NOT NULL DEFAULT '10',
  `Intelligence` tinyint unsigned NOT NULL DEFAULT '10',
  `Magic` tinyint unsigned NOT NULL DEFAULT '10',
  `Agility` tinyint unsigned NOT NULL DEFAULT '10',
  `Luck` smallint unsigned NOT NULL DEFAULT '10',
  `Exp` int unsigned NOT NULL DEFAULT '0',
  `Gender` tinyint unsigned NOT NULL DEFAULT '0',
  `Skin` smallint unsigned NOT NULL DEFAULT '0',
  `HairStyle` smallint unsigned NOT NULL DEFAULT '0',
  `HairColor` smallint unsigned NOT NULL DEFAULT '0',
  `Underwear` smallint unsigned NOT NULL DEFAULT '0',
  `ApprColor` int unsigned NOT NULL DEFAULT '0',
  `Appr1` mediumint NOT NULL DEFAULT '0',
  `Appr2` mediumint NOT NULL DEFAULT '0',
  `Appr3` mediumint NOT NULL DEFAULT '0',
  `Appr4` mediumint NOT NULL DEFAULT '0',
  `Nation` varchar(10) NOT NULL DEFAULT 'Traveler',
  `MapLoc` varchar(10) NOT NULL DEFAULT 'default',
  `LocX` mediumint NOT NULL DEFAULT '-1',
  `LocY` mediumint NOT NULL DEFAULT '-1',
  `Contribution` int unsigned NOT NULL DEFAULT '0',
  `LeftSpecTime` int unsigned NOT NULL DEFAULT '0',
  `LockMapName` varchar(10) NOT NULL DEFAULT 'NONE',
  `LockMapTime` int unsigned NOT NULL DEFAULT '0',
  `BlockDate` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
  `GuildName` varchar(20) NOT NULL DEFAULT 'NONE',
  `GuildID` int NOT NULL DEFAULT '-1',
  `GuildRank` tinyint NOT NULL DEFAULT '-1',
  `FightNum` tinyint NOT NULL DEFAULT '0',
  `FightDate` int unsigned NOT NULL DEFAULT '0',
  `FightTicket` tinyint NOT NULL DEFAULT '0',
  `QuestNum` smallint unsigned NOT NULL DEFAULT '0',
  `QuestID` int unsigned NOT NULL DEFAULT '0',
  `QuestCount` smallint unsigned NOT NULL DEFAULT '0',
  `QuestRewType` smallint NOT NULL DEFAULT '0',
  `QuestRewAmmount` int unsigned NOT NULL DEFAULT '0',
  `QuestCompleted` tinyint(1) NOT NULL DEFAULT '0',
  `EventID` int unsigned NOT NULL DEFAULT '0',
  `WarCon` int unsigned NOT NULL DEFAULT '0',
  `CruJob` smallint unsigned NOT NULL DEFAULT '0',
  `CruID` int unsigned NOT NULL DEFAULT '0',
  `CruConstructPoint` int unsigned NOT NULL DEFAULT '0',
  `Popularity` int NOT NULL DEFAULT '0',
  `HP` int unsigned NOT NULL DEFAULT '0',
  `MP` int unsigned NOT NULL DEFAULT '0',
  `SP` int unsigned NOT NULL DEFAULT '0',
  `EK` int unsigned NOT NULL DEFAULT '0',
  `PK` int unsigned NOT NULL DEFAULT '0',
  `RewardGold` int unsigned NOT NULL DEFAULT '0',
  `DownSkillID` tinyint NOT NULL DEFAULT '-1',
  `Hunger` tinyint unsigned NOT NULL DEFAULT '100',
  `LeftSAC` tinyint unsigned NOT NULL DEFAULT '0',
  `LeftShutupTime` int unsigned NOT NULL DEFAULT '0',
  `LeftPopTime` int unsigned NOT NULL DEFAULT '0',
  `LeftForceRecallTime` int unsigned NOT NULL DEFAULT '0',
  `LeftFirmStaminarTime` int unsigned NOT NULL DEFAULT '0',
  `LeftDeadPenaltyTime` int unsigned NOT NULL DEFAULT '0',
  `PartyID` int unsigned NOT NULL DEFAULT '0',
  `GizonItemUpgradeLeft` int unsigned NOT NULL DEFAULT '0',
  `elo` int unsigned NOT NULL DEFAULT '1000',
  `BankModified` tinyint unsigned NOT NULL DEFAULT '0',
  `AdminLevel` tinyint unsigned NOT NULL DEFAULT '0',
  `MagicMastery` varchar(100) NOT NULL DEFAULT '0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000',
  `Profile` varchar(255) NOT NULL DEFAULT '__________',
  PRIMARY KEY (`char_name`),
  UNIQUE KEY `CharID` (`CharID`),
  KEY `account_name` (`account_name`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=latin1 PACK_KEYS=0;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `char_database`
--

LOCK TABLES `char_database` WRITE;
/*!40000 ALTER TABLE `char_database` DISABLE KEYS */;
INSERT INTO `char_database` VALUES ('CamB','toke',1,5836,436,1,152,169,90,185,59,10,10,10,1762611,2,2,0,11,5,0,181,-3680,0,0,'elvine','elvine',152,34,0,0,'NONE',0,'1000-01-01 00:00:00','NONE',-1,127,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,658,308,606,0,0,0,-1,95,15,0,0,0,0,0,0,0,1000,0,0,'11100000001011100000110010111000000000001100011100100000000000000000000000000000000000000000000000','__________');
/*!40000 ALTER TABLE `char_database` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `guild`
--

DROP TABLE IF EXISTS `guild`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `guild` (
  `GuildName` varchar(20) NOT NULL DEFAULT '',
  `GuildID` int unsigned NOT NULL AUTO_INCREMENT,
  `MasterName` varchar(10) NOT NULL DEFAULT '',
  `Nation` varchar(10) NOT NULL DEFAULT '',
  `NumberOfMembers` smallint unsigned NOT NULL DEFAULT '0',
  `CreateDate` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
  `Captains` smallint unsigned NOT NULL DEFAULT '0',
  `HuntMasters` smallint unsigned NOT NULL DEFAULT '0',
  `RaidMasters` smallint unsigned NOT NULL DEFAULT '0',
  `WHLevel` smallint unsigned NOT NULL DEFAULT '0',
  UNIQUE KEY `GuildID` (`GuildID`),
  KEY `GuildName` (`GuildName`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `guild`
--

LOCK TABLES `guild` WRITE;
/*!40000 ALTER TABLE `guild` DISABLE KEYS */;
/*!40000 ALTER TABLE `guild` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `guild_item`
--

DROP TABLE IF EXISTS `guild_item`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `guild_item` (
  `GuildID` int unsigned NOT NULL DEFAULT '0',
  `ItemName` varchar(20) NOT NULL DEFAULT '',
  `ItemID` bigint unsigned NOT NULL AUTO_INCREMENT,
  `Count` int unsigned NOT NULL DEFAULT '0',
  `ItemType` tinyint unsigned NOT NULL DEFAULT '0',
  `ID1` smallint NOT NULL DEFAULT '0',
  `ID2` smallint NOT NULL DEFAULT '0',
  `ID3` smallint NOT NULL DEFAULT '0',
  `Color` tinyint unsigned NOT NULL DEFAULT '0',
  `Effect1` int DEFAULT '0',
  `Effect2` int DEFAULT '0',
  `Effect3` int DEFAULT '0',
  `LifeSpan` int unsigned NOT NULL DEFAULT '0',
  `Attribute` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`ItemID`),
  UNIQUE KEY `ItemID` (`ItemID`),
  KEY `Index` (`GuildID`),
  KEY `ItemName` (`ItemName`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 PACK_KEYS=0;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `guild_item`
--

LOCK TABLES `guild_item` WRITE;
/*!40000 ALTER TABLE `guild_item` DISABLE KEYS */;
/*!40000 ALTER TABLE `guild_item` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `guild_member`
--

DROP TABLE IF EXISTS `guild_member`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `guild_member` (
  `GuildName` varchar(20) NOT NULL DEFAULT '',
  `MemberName` varchar(10) NOT NULL DEFAULT '',
  `JoinDate` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
  KEY `GuildName` (`GuildName`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `guild_member`
--

LOCK TABLES `guild_member` WRITE;
/*!40000 ALTER TABLE `guild_member` DISABLE KEYS */;
/*!40000 ALTER TABLE `guild_member` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `ipblocked`
--

DROP TABLE IF EXISTS `ipblocked`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `ipblocked` (
  `ipaddress` varchar(20) NOT NULL DEFAULT '',
  `comment` mediumtext,
  PRIMARY KEY (`ipaddress`),
  UNIQUE KEY `ipaddress` (`ipaddress`),
  KEY `ipaddress_2` (`ipaddress`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `ipblocked`
--

LOCK TABLES `ipblocked` WRITE;
/*!40000 ALTER TABLE `ipblocked` DISABLE KEYS */;
/*!40000 ALTER TABLE `ipblocked` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `item`
--

DROP TABLE IF EXISTS `item`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `item` (
  `CharID` int unsigned NOT NULL DEFAULT '0',
  `ItemName` varchar(21) DEFAULT NULL,
  `ItemID` bigint unsigned NOT NULL AUTO_INCREMENT,
  `Count` int unsigned NOT NULL DEFAULT '1',
  `ItemType` tinyint unsigned NOT NULL DEFAULT '0',
  `ID1` smallint NOT NULL DEFAULT '0',
  `ID2` smallint NOT NULL DEFAULT '0',
  `ID3` smallint NOT NULL DEFAULT '0',
  `Color` tinyint unsigned NOT NULL DEFAULT '0',
  `Effect1` int DEFAULT '0',
  `Effect2` int DEFAULT '0',
  `Effect3` int DEFAULT '0',
  `LifeSpan` int unsigned NOT NULL DEFAULT '0',
  `Attribute` int unsigned NOT NULL DEFAULT '0',
  `ItemEquip` tinyint(1) NOT NULL DEFAULT '0',
  `ItemPosX` int NOT NULL DEFAULT '40',
  `ItemPosY` int NOT NULL DEFAULT '30',
  PRIMARY KEY (`ItemID`),
  UNIQUE KEY `ItemID` (`ItemID`),
  KEY `Index` (`CharID`),
  KEY `ItemName` (`ItemName`)
) ENGINE=InnoDB AUTO_INCREMENT=1156 DEFAULT CHARSET=latin1 PACK_KEYS=0;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `item`
--

LOCK TABLES `item` WRITE;
/*!40000 ALTER TABLE `item` DISABLE KEYS */;
INSERT INTO `item` VALUES (1,'Dagger',1,1,2,0,0,0,0,0,0,0,300,0,0,30,30),(1,'BigRedPotion',2,1,0,0,0,0,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',3,1,0,0,0,0,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',4,1,0,0,0,0,0,0,0,0,1,0,0,161,95),(1,'BigBluePotion',5,1,0,0,0,0,0,0,0,0,1,0,0,45,30),(1,'BigBluePotion',6,1,0,0,0,0,0,0,0,0,1,0,0,45,30),(1,'BigBluePotion',7,1,0,0,0,0,0,0,0,0,1,0,0,45,30),(1,'BigGreenPotion',8,1,0,0,0,0,0,0,0,0,1,0,0,0,95),(1,'BigGreenPotion',9,1,0,0,0,0,0,0,0,0,1,0,0,0,95),(1,'BigGreenPotion',10,1,0,0,0,0,0,0,0,0,1,0,0,0,95),(1,'Map',11,1,0,0,0,0,0,0,0,0,1,0,0,122,91),(1,'Gold',12,19596,0,0,0,0,0,0,0,0,0,0,0,155,54),(1,'ShortSword',33,1,2,20848,17175,218,7,0,0,0,488,3211264,0,39,66),(1,'BigRedPotion',1114,1,2,12911,24936,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1115,1,2,13273,31780,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1116,1,2,2910,28820,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1117,1,2,26292,14271,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1118,1,2,31817,20949,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1119,1,2,18726,5896,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1120,1,2,32464,19554,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1121,1,2,10589,1033,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1122,1,2,5916,19658,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1123,1,2,5754,4842,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1124,1,2,1035,25737,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1125,1,2,12766,7712,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1126,1,2,14989,26586,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1127,1,2,9636,29409,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1128,1,2,12366,7629,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1129,1,2,25710,8667,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1130,1,2,14741,13977,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1131,1,2,5733,18469,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1132,1,2,32364,31795,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1133,1,2,22395,25718,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1134,1,2,19201,28960,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1135,1,2,10996,3221,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1136,1,2,16204,20871,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1137,1,2,18468,5904,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1138,1,2,8690,10890,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1139,1,2,9338,3377,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1140,1,2,23145,31702,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1141,1,2,24253,31022,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1142,1,2,22109,6009,219,0,0,0,0,1,0,0,161,95),(1,'BigRedPotion',1143,1,2,28566,25760,219,0,0,0,0,1,0,0,161,95),(1,'ShortSword',1148,1,2,25064,22552,219,6,0,0,0,500,7405568,0,83,35),(1,'BattleAxe',1149,1,2,31401,3243,219,0,0,0,0,2730,0,1,70,39),(1,'MagicWand(MS20)',1150,1,2,26536,11841,219,0,0,0,0,3600,0,0,150,15),(1,'WoodShield',1151,1,2,27573,25333,219,0,0,0,0,700,8519680,0,40,30),(1,'WoodShield',1152,1,2,30608,14111,219,0,0,0,0,700,6619136,0,40,30),(1,'TargeShield',1154,1,2,694,20734,219,0,0,0,0,1000,8540416,0,40,30),(1,'TargeShield',1155,1,2,17004,13816,219,0,0,0,0,1000,6578432,0,40,30);
/*!40000 ALTER TABLE `item` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `paypal_cart_info`
--

DROP TABLE IF EXISTS `paypal_cart_info`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `paypal_cart_info` (
  `txnid` varchar(30) NOT NULL DEFAULT '',
  `itemname` varchar(255) NOT NULL DEFAULT '',
  `itemnumber` varchar(50) DEFAULT NULL,
  `os0` varchar(20) DEFAULT NULL,
  `on0` varchar(50) DEFAULT NULL,
  `os1` varchar(20) DEFAULT NULL,
  `on1` varchar(50) DEFAULT NULL,
  `quantity` char(3) NOT NULL DEFAULT '',
  `invoice` varchar(255) NOT NULL DEFAULT '',
  `custom` varchar(255) NOT NULL DEFAULT ''
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `paypal_cart_info`
--

LOCK TABLES `paypal_cart_info` WRITE;
/*!40000 ALTER TABLE `paypal_cart_info` DISABLE KEYS */;
/*!40000 ALTER TABLE `paypal_cart_info` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `paypal_payment_info`
--

DROP TABLE IF EXISTS `paypal_payment_info`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `paypal_payment_info` (
  `firstname` varchar(100) NOT NULL DEFAULT '',
  `lastname` varchar(100) NOT NULL DEFAULT '',
  `buyer_email` varchar(100) NOT NULL DEFAULT '',
  `memo` varchar(255) DEFAULT NULL,
  `itemname` varchar(255) DEFAULT NULL,
  `itemnumber` varchar(50) DEFAULT NULL,
  `os0` varchar(20) DEFAULT NULL,
  `on0` varchar(50) DEFAULT NULL,
  `os1` varchar(20) DEFAULT NULL,
  `on1` varchar(50) DEFAULT NULL,
  `quantity` char(3) DEFAULT NULL,
  `paymentdate` varchar(50) NOT NULL DEFAULT '',
  `paymenttype` varchar(10) NOT NULL DEFAULT '',
  `txnid` varchar(30) NOT NULL DEFAULT '',
  `mc_gross` varchar(6) NOT NULL DEFAULT '',
  `mc_fee` varchar(5) NOT NULL DEFAULT '',
  `paymentstatus` varchar(15) NOT NULL DEFAULT '',
  `pendingreason` varchar(10) DEFAULT NULL,
  `txntype` varchar(10) NOT NULL DEFAULT '',
  `tax` varchar(10) DEFAULT NULL,
  `mc_currency` varchar(5) NOT NULL DEFAULT '',
  `reasoncode` varchar(20) NOT NULL DEFAULT '',
  `custom` varchar(255) NOT NULL DEFAULT '',
  `street` varchar(100) NOT NULL,
  `city` varchar(50) NOT NULL,
  `zipcode` varchar(11) NOT NULL,
  `state` char(15) NOT NULL,
  `country` varchar(20) NOT NULL DEFAULT '',
  `datecreation` date NOT NULL DEFAULT '0000-00-00'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `paypal_payment_info`
--

LOCK TABLES `paypal_payment_info` WRITE;
/*!40000 ALTER TABLE `paypal_payment_info` DISABLE KEYS */;
/*!40000 ALTER TABLE `paypal_payment_info` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `paypal_subscription_info`
--

DROP TABLE IF EXISTS `paypal_subscription_info`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `paypal_subscription_info` (
  `subscr_id` varchar(255) NOT NULL DEFAULT '',
  `sub_event` varchar(50) NOT NULL DEFAULT '',
  `subscr_date` varchar(255) NOT NULL DEFAULT '',
  `subscr_effective` varchar(255) NOT NULL DEFAULT '',
  `period1` varchar(255) NOT NULL DEFAULT '',
  `period2` varchar(255) NOT NULL DEFAULT '',
  `period3` varchar(255) NOT NULL DEFAULT '',
  `amount1` varchar(255) NOT NULL DEFAULT '',
  `amount2` varchar(255) NOT NULL DEFAULT '',
  `amount3` varchar(255) NOT NULL DEFAULT '',
  `mc_amount1` varchar(255) NOT NULL DEFAULT '',
  `mc_amount2` varchar(255) NOT NULL DEFAULT '',
  `mc_amount3` varchar(255) NOT NULL DEFAULT '',
  `recurring` varchar(255) NOT NULL DEFAULT '',
  `reattempt` varchar(255) NOT NULL DEFAULT '',
  `retry_at` varchar(255) NOT NULL DEFAULT '',
  `recur_times` varchar(255) NOT NULL DEFAULT '',
  `username` varchar(255) NOT NULL DEFAULT '',
  `password` varchar(255) DEFAULT NULL,
  `payment_txn_id` varchar(50) NOT NULL DEFAULT '',
  `subscriber_emailaddress` varchar(255) NOT NULL DEFAULT '',
  `datecreation` date NOT NULL DEFAULT '0000-00-00'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `paypal_subscription_info`
--

LOCK TABLES `paypal_subscription_info` WRITE;
/*!40000 ALTER TABLE `paypal_subscription_info` DISABLE KEYS */;
/*!40000 ALTER TABLE `paypal_subscription_info` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `skill`
--

DROP TABLE IF EXISTS `skill`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `skill` (
  `CharID` int unsigned NOT NULL DEFAULT '0',
  `SkillID` tinyint unsigned NOT NULL DEFAULT '0',
  `SkillMastery` tinyint unsigned NOT NULL DEFAULT '0',
  `SkillSSN` int unsigned NOT NULL DEFAULT '0',
  KEY `Index` (`CharID`),
  KEY `SkillID` (`SkillID`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `skill`
--

LOCK TABLES `skill` WRITE;
/*!40000 ALTER TABLE `skill` DISABLE KEYS */;
INSERT INTO `skill` VALUES (1,0,1,0),(1,1,1,0),(1,2,1,0),(1,3,1,0),(1,4,15,0),(1,5,1,0),(1,6,1,0),(1,7,11,5),(1,8,1,0),(1,9,1,0);
/*!40000 ALTER TABLE `skill` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2026-02-19 23:13:00
