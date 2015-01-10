#include "Environment.h"

#include <String.h>
#include <stdlib.h>

Environment::Environment()
{
	env = (char **)malloc(sizeof(char *) * 128);
	blocks = 128;
	next = 0;
}


Environment::Environment(char **envir)
{
	env = envir;
	blocks = 0;
	while (*envir != 0) {
		envir++;
		blocks++;
	}
	next = blocks;
}


Environment::~Environment() {}

const char *
Environment::GetEnv(const char *name, int *index) {
	BString search = name;
	search.Append("=");
	int32 l = search.Length();

	for (int i = 0; i < blocks; i++) {
		if (strncmp(search.String(), env[i], l) == 0) {
			if (index)
				*index = i;
			return env[i];
		}
	}
}


const char *
Environment::GetEnv(int index) {
	return env[index];
}


void
Environment::PutEnv(const char *string) {
	int index;
	BString name(string, strchr(string, '=') - string);
	if (GetEnv(name, &index)) {
		free(env[index]);
	} else {
		if (blocks == next) {
			env = (char **)realloc(env, blocks + 16);
			blocks += 16;
		}
		index = next++;
	}
	env[index] = (char *)malloc(strlen(string) + 1);
	strcpy(env[index], string);
}


void
Environment::PutEnv(const char *name, const char *value)
{
	BString str;
	str.SetToFormat("%s=%s", name, value);
	PutEnv(str);
}


char **
Environment::GetEnvironment()
{
	return env;
}


void
Environment::MakeEmpty()
{
	for (int i = 0; i < next; i++) {
		free(env[i]);
	}

	next = 0;
}


int
Environment::CountVariables()
{
	return next;
}


void
Environment::CopyEnv(char **env)
{
	// meh.
}


void
Environment::InitEnvironment()
{
	// nothing here
}


void
Environment::AllocateBlock()
{
	// this function purposely left blank
}
