/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Process list functions, rather dull.
 */

#include <types.h>
#include <lib.h>
#include <proc.h>
#include <proclist.h>

void
proclistnode_init(struct proclistnode *pln, struct proc *p)
{
	DEBUGASSERT(pln != NULL);
	KASSERT(p != NULL);

	pln->pln_next = NULL;
	pln->pln_prev = NULL;
	pln->pln_self = p;
}

void
proclistnode_cleanup(struct proclistnode *pln)
{
	DEBUGASSERT(pln != NULL);

	KASSERT(pln->pln_next == NULL);
	KASSERT(pln->pln_prev == NULL);
	KASSERT(pln->pln_self != NULL);
}

void
proclist_init(struct proclist *pl)
{
	DEBUGASSERT(pl != NULL);

	pl->pl_head.pln_next = &pl->pl_tail;
	pl->pl_head.pln_prev = NULL;
	pl->pl_tail.pln_next = NULL;
	pl->pl_tail.pln_prev = &pl->pl_head;
	pl->pl_head.pln_self = NULL;
	pl->pl_tail.pln_self = NULL;
	pl->pl_count = 0;
}

void
proclist_cleanup(struct proclist *pl)
{
	DEBUGASSERT(pl != NULL);
	DEBUGASSERT(pl->pl_head.pln_next == &pl->pl_tail);
	DEBUGASSERT(pl->pl_head.pln_prev == NULL);
	DEBUGASSERT(pl->pl_tail.pln_next == NULL);
	DEBUGASSERT(pl->pl_tail.pln_prev == &pl->pl_head);
	DEBUGASSERT(pl->pl_head.pln_self == NULL);
	DEBUGASSERT(pl->pl_tail.pln_self == NULL);

	KASSERT(proclist_isempty(pl));
	KASSERT(pl->pl_count == 0);

	/* nothing (else) to do */
}

bool
proclist_isempty(struct proclist *pl)
{
	DEBUGASSERT(pl != NULL);

	return (pl->pl_count == 0);
}

////////////////////////////////////////////////////////////
// internal

/*
 * Do insertion. Doesn't update tl_count.
 */
static
void
proclist_insertafternode(struct proclistnode *onlist, struct proc *p)
{
	struct proclistnode *addee;

	addee = &p->p_listnode;

	DEBUGASSERT(addee->pln_prev == NULL);
	DEBUGASSERT(addee->pln_next == NULL);

	addee->pln_prev = onlist;
	addee->pln_next = onlist->pln_next;
	addee->pln_prev->pln_next = addee;
	addee->pln_next->pln_prev = addee;
}

/*
 * Do insertion. Doesn't update tl_count.
 */
static
void
proclist_insertbeforenode(struct proc *p, struct proclistnode *onlist)
{
	struct proclistnode *addee;

	addee = &p->p_listnode;

	DEBUGASSERT(addee->pln_prev == NULL);
	DEBUGASSERT(addee->pln_next == NULL);

	addee->pln_prev = onlist->pln_prev;
	addee->pln_next = onlist;
	addee->pln_prev->pln_next = addee;
	addee->pln_next->pln_prev = addee;
}

/*
 * Do removal. Doesn't update tl_count.
 */
static
void
proclist_removenode(struct proclistnode *pln)
{
	DEBUGASSERT(pln != NULL);
	DEBUGASSERT(pln->pln_prev != NULL);
	DEBUGASSERT(pln->pln_next != NULL);

	pln->pln_prev->pln_next = pln->pln_next;
	pln->pln_next->pln_prev = pln->pln_prev;
	pln->pln_prev = NULL;
	pln->pln_next = NULL;
}

////////////////////////////////////////////////////////////
// public

void
proclist_addhead(struct proclist *pl, struct proc *p)
{
	DEBUGASSERT(pl != NULL);
	DEBUGASSERT(p != NULL);

	proclist_insertafternode(&pl->pl_head, p);
	pl->pl_count++;
}

void
proclist_addtail(struct proclist *pl, struct proc *p)
{
	DEBUGASSERT(pl != NULL);
	DEBUGASSERT(p != NULL);

	proclist_insertbeforenode(p, &pl->pl_tail);
	pl->pl_count++;
}

struct proc *
proclist_remhead(struct proclist *pl)
{
	struct proclistnode *pln;

	DEBUGASSERT(pl != NULL);

	pln = pl->pl_head.pln_next;
	if (pln->pln_next == NULL) {
		/* list was empty  */
		return NULL;
	}
	proclist_removenode(pln);
	DEBUGASSERT(pl->pl_count > 0);
	pl->pl_count--;
	return pln->pln_self;
}

struct proc *
proclist_remtail(struct proclist *pl)
{
	struct proclistnode *pln;

	DEBUGASSERT(pl != NULL);

	pln = pl->pl_tail.pln_prev;
	if (pln->pln_prev == NULL) {
		/* list was empty  */
		return NULL;
	}
	proclist_removenode(pln);
	DEBUGASSERT(pl->pl_count > 0);
	pl->pl_count--;
	return pln->pln_self;
}

void
proclist_insertafter(struct proclist *pl,
		       struct proc *onlist, struct proc *addee)
{
	proclist_insertafternode(&onlist->p_listnode, addee);
	pl->pl_count++;
}

void
proclist_insertbefore(struct proclist *pl,
			struct proc *addee, struct proc *onlist)
{
	proclist_insertbeforenode(addee, &onlist->p_listnode);
	pl->pl_count++;
}

void
proclist_remove(struct proclist *pl, struct proc *p)
{
	proclist_removenode(&p->p_listnode);
	DEBUGASSERT(pl->pl_count > 0);
	pl->pl_count--;
}
