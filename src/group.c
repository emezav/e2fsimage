/* vi: set sw=4 ts=4: */
/*
 * Copyright (C) 2001 Christian Hohnstaedt
 * Copyright (C) 2006 rPath, Inc.
 *
 *  All rights reserved.
 *
 *
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  - Neither the name of the author nor the names of its contributors may be 
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 *
 * http://www.hohnstaedt.de/e2fsimage
 * email: christian@hohnstaedt.de
 * email: jtate@rpath.com
 *
 * $Id: group.c,v 1.2 2006/02/04 09:42:41 chris2511 Exp $ 
 *
 */                           

#include "e2fsimage.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>
			
/*
 * read the etc/group file
 */

int read_group(e2i_ctx_t *e2c)
{
	FILE *fp;
	char line_buf[256], *p1, *p2;
	int ln=0, gid, len;

	/* try to open the file or return */
	fp = fopen(e2c->grp_file, "r");
	if(!fp){
		return 0;
	}
	
	if (verbose)
		printf("Reading username information from %s\n", e2c->grp_file);

	/* iterate over the lines in the device file */
	while (fgets(line_buf, 256, fp) != 0) {
		ln++;  /* count the line numbers */
		/* check for too long lines */
		if (strlen(line_buf)>254) {
			char c = line_buf[254];
			fprintf(stderr, "Line too long %d\n",ln);
			/* eat up the rest of the line */
			while (c != '\n' && c >0) c = fgetc(fp);
			continue;
		}
		
		do {
			gid = -1;
			p1  = strchr(line_buf, ':'); /* the : between group and password */
			len = p1 - line_buf;
			if (len > 79 || len < 1) break;
			*p1 = '\0'; /* terminate name */
			p1 = strchr(p1+1,':'); /* points to : before GID */
			if (!p1) break;
			p2 = strchr(++p1,':'); /* points to the colon after the GID */
			if (!p2) break;
			*p2++ = '\0'; /* delete : and point to the userlist if any */
			gid = atoi(p1);
		} while(0);
		
		if (gid < 0) {
			fprintf(stderr, "Bad entry in %s, line %d : %s, %d\n",
				e2c->grp_file, ln, line_buf, gid);
			fclose(fp);
			return -1;
		}
		uiddb_add(e2c->group, line_buf, 0, gid);
	}
	fclose(fp);
	return 0;
}

