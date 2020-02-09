/*
   Drawpile - a collaborative drawing program.

   Copyright (C) 2017 Calle Laakkonen

   Drawpile is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Drawpile is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Drawpile.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "dblog.h"

#include <QSqlQuery>
#include <QMetaEnum>
#include <QSqlError>

namespace server {

DbLog::DbLog(const QSqlDatabase &db)
	: m_db(db)
{
}

bool DbLog::initDb()
{
	QSqlQuery q(m_db);
	return q.exec(
		"CREATE TABLE IF NOT EXISTS serverlog ("
			"timestamp, level, topic, user, session, message"
		");"
	);
}

QList<Log> DbLog::getLogEntries(const QString &session, const QDateTime &after, Log::Level atleast, int offset, int limit) const
{
	QString sql = "SELECT timestamp, session, user, level, topic, message FROM serverlog WHERE 1=1";
	QVariantList params;
	if(!session.isEmpty()) {
		sql += " AND session=?";
		params << session;
	}
	if(after.isValid()) {
		sql += " AND timestamp>=?";
		params << after.addMSecs(1000).toString(Qt::ISODate);
	}

	if(atleast < Log::Level::Debug) {
		sql += " AND level<=?";
		params << int(atleast);
	}

	sql += " ORDER BY timestamp DESC, rowid DESC";

	if(limit>0) {
		sql += " LIMIT ?";
		params << limit;
	}
	if(offset>0) {
		sql += " OFFSET ?";
		params << offset;
	}

	QSqlQuery q(m_db);
	q.prepare(sql);
	for(int i=0;i<params.size();++i)
		q.bindValue(i, params.at(i));

	if(!q.exec()) {
		qDebug("exec: %s", qPrintable(q.executedQuery()));
		qWarning("Database log query error: %s", qPrintable(q.lastError().text()));
	}

	QList<Log> results;
	while(q.next()) {
		results << Log(
			q.value(0).toDateTime(),
			q.value(1).toString(),
			q.value(2).toString(),
			Log::Level(q.value(3).toInt()),
			Log::Topic(QMetaEnum::fromType<Log::Topic>().keyToValue(q.value(4).toString().toLocal8Bit().constData())),
			q.value(5).toString()
		);
	}
	return results;
}

void DbLog::storeMessage(const Log &entry)
{
	QSqlQuery q(m_db);
	q.prepare("INSERT INTO serverlog (timestamp, level, topic, user, session, message) VALUES (?, ?, ?, ?, ?, ?)");
	q.bindValue(0, entry.timestamp().toString(Qt::ISODate));
	q.bindValue(1, int(entry.level()));
	q.bindValue(2, QMetaEnum::fromType<Log::Topic>().valueToKey(int(entry.topic())));
	q.bindValue(3, entry.user());
	q.bindValue(4, entry.session());
	q.bindValue(5, entry.message());
	q.exec();
}

int DbLog::purgeLogs(int olderThanDays)
{
	if(olderThanDays<=0)
		return 0;

	QSqlQuery q(m_db);
	q.prepare("DELETE FROM serverlog WHERE timestamp < DATE('now', ?)");
	q.bindValue(0, QStringLiteral("-%1 days").arg(olderThanDays));
	if(!q.exec())
		qWarning("Couldn't purge log entries: %s", qPrintable(q.lastError().databaseText()));
	return q.numRowsAffected();
}

}
