/*
 *  util.c for Fwife
 *
 *  Copyright (c) 2009,2010 by Albar Boris <boris.a@cegetel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *  USA.
 */

#ifndef _MSG_H_INCLUDED
#define _MSG_H_INCLUDED

/* Some functions for basic messages */
void fwife_fatal_error(const char *msg);
void fwife_error(const char *msg);
void fwife_info(const char *msg);
int fwife_question(const char *msg);
char* fwife_entry(const char *, const char*, const char*);

#endif /* _MSG_H_INCLUDED */
