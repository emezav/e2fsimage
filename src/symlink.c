/* vi: set sw=4 ts=4: */
/*
 * Copyright (C) 2001 Christian Hohnstaedt.
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
 *
 * $Id: symlink.c,v 1.3 2004/01/17 22:13:59 chris2511 Exp $ 
 *
 */                           

#include "e2fsimage.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define BUF_SIZE 256

int e2symlink(ext2_filsys fs, ext2_ino_t e2dir, const char *pathlink) 
{
	
	ext2_file_t e2file;
	ext2_ino_t e2ino;
	struct ext2_inode inode;
	int ret, written;
	char buf[BUF_SIZE];
	const char *fname;
	off_t size = 0;
	struct stat s;
	
	/* 'stat' the file we want to copy */
	ret = lstat(pathlink, &s);
	ERRNO_ERR(ret, "Could not 'stat': ", pathlink);
			
	if (!S_ISLNK(s.st_mode)) {
		fprintf(stderr, "File '%s' is not a symlink file\n", pathlink);
		return -1;
	}

	/* create a new inode for this file */
	ret = ext2fs_new_inode(fs, e2dir, s.st_mode, 0, &e2ino);
	E2_ERR(ret, "Could not create new inode for: ", pathlink);
	
	/* populate the new inode */
	ext2fs_inode_alloc_stats(fs, e2ino, 1);
	
	init_inode(&inode, &s);

	ret = ext2fs_write_inode(fs, e2ino, &inode);
	E2_ERR(ret, "could not write inode", "");
	
	/* open the targetfile */
	ret = ext2fs_file_open(fs, e2ino, EXT2_FILE_WRITE, &e2file);
	E2_ERR(ret, "file open error", "");

	/* open the source file */
	size = readlink(pathlink, buf, BUF_SIZE);
	if (size < 0 || size >= BUF_SIZE) {
		fprintf(stderr, "Error reading symlink '%s': %s\n", pathlink, strerror(errno));
		return -1;
	}
	
	if (verbose)
		printf("Copying symlink %s\n",pathlink);
	
	ret = ext2fs_file_write(e2file, buf, size, &written);
	if (ret) {
		fprintf(stderr, "Error writing ext2 symlink (%s)\n", error_message(ret));
		ext2fs_file_close(e2file);
		return ret;
	}

	ext2fs_file_close(e2file);
	
	/* if this sizes differ its an inconsistency in the base filesystem */
	if (size != written) {
		fprintf(stderr, "Error 'size matters' Size:%ld, Written:%d\n", size, written);
		return -1;
	}
	
	fname = basename(pathlink);
	
	/* It is time to link the inode into the directory */
	ret = ext2fs_link(fs, e2dir, fname, e2ino, EXT2_FT_SYMLINK);
	if (ret == EXT2_ET_DIR_NO_SPACE) {
		/* resize the directory */
		if (ext2fs_expand_dir(fs, e2dir) == 0)
			ret = ext2fs_link(fs, e2dir, fname, e2ino, EXT2_FT_SYMLINK);
	}			  
	
	E2_ERR(ret, "e2-link error", "");

	return 0;	
}