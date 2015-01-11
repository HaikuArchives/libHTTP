#include "HTTPRealm.h"
#include "StringUtils.h"

#include <stdlib.h>
#include <fnmatch.h>

HTTPRealm::HTTPRealm()
{
	// nothing
}


HTTPRealm::~HTTPRealm()
{
	for (int32 i = 0; i < CountItems(); i++)
		delete (BString *)ItemAt(i);
}


void
HTTPRealm::SetName(const char *name)
{
	this->name = name;
}


void
HTTPRealm::SetUser(const char *user)
{
	this->user = user;
}


void
HTTPRealm::SetPasswd(const char *passwd)
{
	this->passwd = passwd;
}


void
HTTPRealm::AddPattern(const char *pattern)
{
	AddItem(new BString(pattern));
}


bool
HTTPRealm::Match(const char *path)
{
	for (int32 i = 0; i < CountItems(); i++) {
		BString *p = (BString *)ItemAt(i);
		if (fnmatch(p->String(), path, 0) == 0) {
			return true;
		}
	}

	return false;
}


const char *
HTTPRealm::GetName()
{
	return name;
}


const char *
HTTPRealm::GetPasswd()
{
	return passwd;
}


const char *
HTTPRealm::GetUser()
{
	return user;
}


HTTPRealmList::HTTPRealmList()
{
	
}


HTTPRealmList::~HTTPRealmList()
{
	for (int32 i = 0; i < CountItems(); i++) {
		HTTPRealm *r = (HTTPRealm *)ItemAt(i);
		delete r;
	}
	
}


void
HTTPRealmList::SetDefaultRealmName(const char *name)
{
	defaultRealm = name;
}


void
HTTPRealmList::AddRealm(HTTPRealm *realm)
{
	AddItem(realm);
}


HTTPRealm *
HTTPRealmList::MatchRealm(const char *path)
{
	for (int32 i = 0; i < CountItems(); i++) {
		HTTPRealm *r = (HTTPRealm *)ItemAt(i);
		if (r->Match(path))
			return r;
	}
	
	return NULL;
}


HTTPRealm *
HTTPRealmList::FindRealm(const char *name)
{
	for (int32 i = 0; i < CountItems(); i++) {
		HTTPRealm *r = (HTTPRealm *)ItemAt(i);
		if (strcmp(r->GetName(), name) == 0)
			return r;
	}
	
	return NULL;
}


bool
HTTPRealmList::Authenticate(HTTPRequest *req, HTTPResponse *res, const char *webPath, const char *absPath, mode_t bitMask)
{
	HTTPRealm *r = MatchRealm(webPath);

	if (!r)
		return true;

	const char *auth = req->FindHeader("Authorization");
	if (auth) {
		auth += 14;
		while (*auth == ' ') auth++;

		if (strncmp(auth, "Basic ", 6) == 0) {
			char namepass[512];
			size_t length = 0;
			length = decode_base64(namepass, auth + 6, strlen(auth + 6));
			if (length == 0)
				return false;
			
			int split = (int)(strchr(namepass, ':') - (char *)namepass);
			BString name(namepass, split);
			BString pass(namepass + split + 1);
			if (name == r->GetUser() && pass == r->GetPasswd())
				return true;
		}
	}

	unath:
		BString realmheader("Basic realm=\"");
		realmheader.Append(r->GetName());
		realmheader.Append("\"");

		res->AddHeader("WWW-Authenticate", realmheader);
		res->SetHTMLMessage(401);
		req->SendReply(res);
		return false;
}
