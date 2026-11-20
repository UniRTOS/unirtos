/*************************************************************
 * Author:	Lionfore Hao (haolianfu@agora.io)
 * Date	 :	Aug 2nd, 2018
 * Module:	Agora SD-RTN RTC SDK version implementations.
 *
 *
 * This is a part of the Agora Media SDK.
 * Copyright (C) 2018 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#include <kernel/kernel.h>
#include <api/aosl_version.h>

#ifndef AOSL_GIT_BRANCH
#define AOSL_GIT_BRANCH "Unknown_Branch"
#endif

#ifndef AOSL_GIT_COMMIT
#define AOSL_GIT_COMMIT "Unknown_Commit"
#endif

__export_in_so__ const char *aosl_get_git_branch ()
{
	return AOSL_GIT_BRANCH;
}

__export_in_so__ const char *aosl_get_git_commit ()
{
	return AOSL_GIT_COMMIT;
}