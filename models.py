#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#~ import os, sys, stat
#~ from datetime import datetime

import sqlalchemy
from sqlalchemy import create_engine
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy import Column, Integer, String, DateTime, ForeignKey
from sqlalchemy.orm import sessionmaker, relationship, backref
from sqlalchemy_utils import database_exists, create_database

from config import cfg
#~ print(cfg)
logd = cfg.logd

fn_database = cfg.fn_database

_Base = declarative_base()

class FSItemFolder(_Base):		# folder have inode (renaming!!)
	__tablename__ = "folders"

	id			= Column(Integer, primary_key = True, nullable=False)
	name		= Column(String, nullable=False, unique=True)
	files		= relationship('FSItem', backref='files', lazy='dynamic')

	def __str__(self):
		res="%s {"%(self.__class__)
		for attr in dir(self):
			if not attr[0]=="_" and not callable(getattr(self, attr)):
				res += "\n\t%s = %s"%(attr, getattr(self, attr))
		res += "\n%s }"%(self.__class__)
		return res

# end FSItemFolder class ===


class FSItemType(_Base):
	__tablename__ = "types"

	id			= Column(Integer, primary_key = True, nullable=False)
	name		= Column(String, nullable=False, unique=True)
	fsitems		= relationship('FSItem', backref='fsitems', lazy='dynamic')

	def __str__(self):
		res="%s {"%(self.__class__)
		for attr in dir(self):
			if not attr[0]=="_" and not callable(getattr(self, attr)):
				res += "\n\t%s = %s"%(attr, getattr(self, attr))
		res += "\n%s }"%(self.__class__)
		return res

# end FSItemType class ===


class FSItem(_Base):
	__tablename__ = "fsitems"

	id			= Column(Integer, primary_key = True, nullable=False)
	folder_id	= Column(Integer, ForeignKey("folders.id"), nullable=False)
	name		= Column(String, nullable=False)
	inode		= Column(Integer, nullable=False)
	nlink		= Column(Integer, nullable=False)
	dev			= Column(Integer, nullable=False)
	stime		= Column(DateTime, nullable=False)
	dtime		= Column(DateTime)					# deletion time
	size		= Column(Integer, nullable=False)
	type_id		= Column(Integer, ForeignKey("types.id"), nullable=False)
	target		= Column(String)	# target of symbolic link
	description	= Column(String)
	atime		= Column(DateTime, nullable=False)
	mtime		= Column(DateTime, nullable=False)
	ctime		= Column(DateTime, nullable=False)

	def __str__(self):
		res="%s {"%(self.__class__)
		for attr in dir(self):
			if not attr[0]=="_" and not callable(getattr(self, attr)):
				res += "\n\t%s = %s"%(attr, getattr(self, attr))
		res += "\n%s }"%(self.__class__)
		return res

# end FSItem class ===

def InitDB():
	global engine

	engine = create_engine(
		"sqlite:///%s?check_same_thread=False"%fn_database,
		echo=False,
		#~ poolclass=StaticPool,
		#~ connect_args={'check_same_thread': False}
	)

	_Base.metadata.create_all(engine)

	if not database_exists(engine.url):
		logd("Creating database: %s", engine.url)
		create_database(engine.url)

	sm = sessionmaker()
	sm.configure(bind=engine)

	session = sm()
	return session


session = InitDB()

def main():
	fsitem = FSItem()
	#~ session.add(fsitem)
	print(fsitem)
	#~ session.commit()

if __name__=='__main__':
	main()
