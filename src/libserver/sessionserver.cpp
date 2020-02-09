/*
   Drawpile - a collaborative drawing program.

   Copyright (C) 2014-2019 Calle Laakkonen

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

#include "sessionserver.h"
#include "thinsession.h"
#include "thinserverclient.h"
#include "loginhandler.h"
#include "serverconfig.h"
#include "serverlog.h"
#include "inmemoryhistory.h"
#include "filedhistory.h"
#include "templateloader.h"
#include "announcements.h"

#include <QTimer>
#include <QJsonArray>
#include <QJsonDocument>

namespace server {

SessionServer::SessionServer(ServerConfig *config, QObject *parent)
	: QObject(parent),
	m_config(config),
	m_tpls(nullptr),
	m_useFiledSessions(false)
{
	m_announcements = new sessionlisting::Announcements(config, this);

	QTimer *cleanupTimer = new QTimer(this);
	connect(cleanupTimer, &QTimer::timeout, this, &SessionServer::cleanupSessions);
	cleanupTimer->setInterval(15 * 1000);
	cleanupTimer->start(cleanupTimer->interval());

#ifndef NDEBUG
	m_randomlag = 0;
#endif
}

void SessionServer::setSessionDir(const QDir &dir)
{
	if(dir.isReadable()) {
		m_sessiondir = dir;
		m_useFiledSessions = true;
		loadNewSessions();
	} else {
		qWarning("%s is not readable", qPrintable(dir.absolutePath()));
	}
}

void SessionServer::loadNewSessions()
{
	if(!m_useFiledSessions)
		return;

	auto sessionFiles = m_sessiondir.entryInfoList(QStringList() << "*.session", QDir::Files|QDir::Writable|QDir::Readable);
	for(const QFileInfo &f : sessionFiles) {
		if(getSessionById(f.baseName(), false))
			continue;

		FiledHistory *fh = FiledHistory::load(f.absoluteFilePath());
		if(fh) {
			fh->setArchive(m_config->getConfigBool(config::ArchiveMode));
			Session *session = new ThinSession(fh, m_config, m_announcements, this);
			initSession(session);
			session->log(Log().about(Log::Level::Debug, Log::Topic::Status).message("Loaded from file."));
		}
	}
}

QJsonArray SessionServer::sessionDescriptions() const
{
	QJsonArray descs;
	QStringList aliases;

	for(const Session *s : m_sessions) {
		descs.append(s->getDescription());
		if(!s->idAlias().isEmpty())
			aliases << s->idAlias();
	}

	if(templateLoader()) {
		// Add session templates to list, if not shadowed by live sessions
		QJsonArray templates = templateLoader()->templateDescriptions();
		for(const QJsonValue &v : templates) {
			if(!aliases.contains(v.toObject().value("alias").toString()))
				descs << v;
		}
	}

	return descs;
}

SessionHistory *SessionServer::initHistory(const QString &id, const QString alias, const protocol::ProtocolVersion &protocolVersion, const QString &founder)
{
	if(m_useFiledSessions) {
		FiledHistory *fh = FiledHistory::startNew(m_sessiondir, id, alias, protocolVersion, founder);
		fh->setArchive(m_config->getConfigBool(config::ArchiveMode));
		return fh;
	} else {
		return new InMemoryHistory(id, alias, protocolVersion, founder);
	}
}

std::tuple<Session*, QString> SessionServer::createSession(const QString &id, const QString &idAlias, const protocol::ProtocolVersion &protocolVersion, const QString &founder)
{
	Q_ASSERT(!id.isNull());

	if(m_sessions.size() >= m_config->getConfigInt(config::SessionCountLimit)) {
		return std::tuple<Session*, QString> { nullptr, "closed" };
	}

	if(getSessionById(id, false) || (!idAlias.isEmpty() && getSessionById(idAlias, false))) {
		return std::tuple<Session*, QString> { nullptr, "idInUse" };
	}

	if(protocolVersion.serverVersion() != protocol::ProtocolVersion::current().serverVersion()) {
		return std::tuple<Session*, QString> { nullptr, "badProtocol" };
	}

	Session *session = new ThinSession(initHistory(id, idAlias, protocolVersion, founder), m_config, m_announcements, this);

	initSession(session);

	QString aka = idAlias.isEmpty() ? QString() : QStringLiteral(" (AKA %1)").arg(idAlias);

	session->log(Log()
		.about(Log::Level::Info, Log::Topic::Status)
		.message("Session" + aka + " created by " + founder));

	return std::make_tuple(session, QString());
}

Session *SessionServer::createFromTemplate(const QString &idAlias)
{
	Q_ASSERT(templateLoader());

	QJsonObject desc = templateLoader()->templateDescription(idAlias);
	if(desc.isEmpty())
		return nullptr;

	SessionHistory *history = initHistory(
		Ulid::make().toString(),
		idAlias, 
		protocol::ProtocolVersion::fromString(desc["protocol"].toString()),
		desc["founder"].toString());

	if(!templateLoader()->init(history)) {
		delete history;
		return nullptr;
	}

	Session *session = new ThinSession(history, m_config, m_announcements, this);
	initSession(session);
	session->log(Log()
		.about(Log::Level::Info, Log::Topic::Status)
		.message(QStringLiteral("Session instantiated from template %1").arg(idAlias)));

	return session;
}

void SessionServer::initSession(Session *session)
{
	m_sessions.append(session);

	const QString idString = session->id();

	connect(session, &Session::sessionAttributeChanged, this, &SessionServer::onSessionAttributeChanged);
	connect(session, &Session::destroyed, this, [this, idString](QObject *object) {
		auto *session = static_cast<Session*>(object);
		m_sessions.removeOne(session);
		m_announcements->unlistSession(session); // just to be safe

		emit sessionEnded(idString);
	});

	emit sessionCreated(session);
	emit sessionChanged(session->getDescription());
}

Session *SessionServer::getSessionById(const QString &id, bool load)
{
	for(Session *s : m_sessions) {
		if(s->id() == id || s->idAlias() == id)
			return s;
	}

	if(load && templateLoader() && templateLoader()->exists(id)) {
		return createFromTemplate(id);
	}

	return nullptr;
}

void SessionServer::stopAll()
{
	for(ThinServerClient *c : m_clients) {
		// Note: this just sends the disconnect command, clients don't self-delete immediately
		c->disconnectClient(Client::DisconnectionReason::Shutdown, "Server shutting down");
	}

	for(Session *s : m_sessions)
		s->killSession(false);
}

void SessionServer::messageAll(const QString &message, bool alert)
{
	for(Session *s : m_sessions) {
		s->messageAll(message, alert);
	}
}

void SessionServer::addClient(ThinServerClient *client)
{
	client->setParent(this);
	client->setConnectionTimeout(m_config->getConfigTime(config::ClientTimeout) * 1000);

#ifndef NDEBUG
	client->setRandomLag(m_randomlag);
#endif

	m_clients.append(client);
	connect(client, &Client::destroyed, this, &SessionServer::removeClient);

	emit userCountChanged(m_clients.size());

	auto *login = new LoginHandler(client, this, m_config);
	connect(this, &SessionServer::sessionChanged, login, &LoginHandler::announceSession);
	connect(this, &SessionServer::sessionEnded, login, &LoginHandler::announceSessionEnd);
	login->startLoginProcess();
}

void SessionServer::removeClient(QObject *client)
{
	m_clients.removeOne(static_cast<ThinServerClient*>(client));
	emit userCountChanged(m_clients.size());
}

/**
 * @brief Handle client disconnect from a session
 *
 * The session takes care of the client itself. Here, we clean up after the session
 * in case it needs to be closed.
 * @param session
 */
void SessionServer::onSessionAttributeChanged(Session *session)
{
	Q_ASSERT(session);

	bool delSession = false;

	if(session->userCount()==0 && session->state() != Session::State::Shutdown) {
		session->log(Log().about(Log::Level::Info, Log::Topic::Status).message("Last user left."));

		// A non-persistent session is deleted when the last user leaves
		// A persistent session can also be deleted if it doesn't contain a snapshot point.
		if(!session->history()->hasFlag(SessionHistory::Persistent)) {
			session->log(Log().about(Log::Level::Info, Log::Topic::Status).message("Closing non-persistent session."));
			delSession = true;
		}
	}

	if(delSession)
		session->killSession();
	else
		emit sessionChanged(session->getDescription());
}

void SessionServer::cleanupSessions()
{
	const qint64 expirationTime = m_config->getConfigTime(config::IdleTimeLimit) * 1000;

	if(expirationTime>0) {
		for(Session *s : m_sessions) {
			if(s->lastEventTime() > expirationTime) {
				s->log(Log().about(Log::Level::Info, Log::Topic::Status).message("Idle session expired."));
				s->killSession();
			}
		}
	}
}

JsonApiResult SessionServer::callSessionJsonApi(JsonApiMethod method, const QStringList &path, const QJsonObject &request)
{
	QString head;
	QStringList tail;
	std::tie(head, tail) = popApiPath(path);

	if(!head.isEmpty()) {
		Session *s = getSessionById(head, false);
		if(s)
			return s->callJsonApi(method, tail, request);
		else
			return JsonApiNotFound();
	}

	if(method == JsonApiMethod::Get) {
		return {JsonApiResult::Ok, QJsonDocument(sessionDescriptions())};

	} else if(method == JsonApiMethod::Update) {
		const QString msg = request["message"].toString();
		if(!msg.isEmpty())
			messageAll(msg, false);

		const QString alert = request["alert"].toString();
		if(!alert.isEmpty())
			messageAll(alert, true);

		return {JsonApiResult::Ok, QJsonDocument(QJsonObject())};


	} else {
		return JsonApiBadMethod();
	}
}

JsonApiResult SessionServer::callUserJsonApi(JsonApiMethod method, const QStringList &path, const QJsonObject &request)
{
	Q_UNUSED(request)

	if(path.size()!=0)
		return JsonApiNotFound();

	if(method == JsonApiMethod::Get) {
		QJsonArray userlist;
		for(const ThinServerClient *c : m_clients)
			userlist << c->description();

		return {JsonApiResult::Ok, QJsonDocument(userlist)};

	} else {
		return JsonApiBadMethod();
	}
}

}
