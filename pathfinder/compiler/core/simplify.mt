/* -*- mode:C; c-basic-offset:4; c-indentation-style:"k&r"; indent-tabs-mode:nil -*-*/

prologue {

/*
 * Simplification for XQuery Core.
 *
 * Copyright Notice:
 * -----------------
 *
 *  The contents of this file are subject to the MonetDB Public
 *  License Version 1.0 (the "License"); you may not use this file
 *  except in compliance with the License. You may obtain a copy of
 *  the License at http://monetdb.cwi.nl/Legal/MonetDBLicense-1.0.html
 *
 *  Software distributed under the License is distributed on an "AS
 *  IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 *  implied. See the License for the specific language governing
 *  rights and limitations under the License.
 *
 *  The Original Code is the ``Pathfinder'' system. The Initial
 *  Developer of the Original Code is the Database & Information
 *  Systems Group at the University of Konstanz, Germany. Portions
 *  created by U Konstanz are Copyright (C) 2000-2004 University
 *  of Konstanz. All Rights Reserved.
 *
 *  Contributors:
 *          Torsten Grust <torsten.grust@uni-konstanz.de>
 *          Maurice van Keulen <M.van.Keulen@bigfoot.com>
 *          Jens Teubner <jens.teubner@uni-konstanz.de>
 *
 * $Id$	
 */

/*
 * NOTE (Revision Information):
 *
 * Changes in the Core2MIL_Summer2004 branch have been merged into
 * this file on July 15, 2004. I have tagged this file in the
 * Core2MIL_Summer2004 branch with `merged-into-main-15-07-2004'.
 *
 * For later merges from the Core2MIL_Summer2004, please only merge
 * the changes since this tag.
 *
 * Jens
 */


#include "pathfinder.h"

/* Auxiliary routines related to the formal semantics are located
 * in this separate included file to facilitate automated documentation
 * via doxygen.
 */
#include "simpl_impl.c"

};

node  var_
      lit_str
      lit_int
      lit_dec
      lit_dbl
      nil

      seq

      let
      for_

      apply
      arg

      typesw
      cases
      case_
      seqtype
      seqcast
      proof
      stattype

      ifthenelse

      locsteps

      ancestor
      ancestor_or_self
      attribute
      child_
      descendant
      descendant_or_self
      following
      following_sibling
      parent_
      preceding
      preceding_sibling
      self

      kind_node
      kind_comment
      kind_text
      kind_pi
      kind_doc
      kind_elem
      kind_attr

      namet

      elem
      attr 
      text
      doc 
      comment
      pi  
      tag

      true_
      false_
      error
      root_
      empty_
;

label Query
      CoreExpr
      BindingExpr
      TypeswitchExpr
      SequenceType
      SequenceTypeCast
      SubtypingProof
      ConditionalExpr
      SequenceExpr
      OptVar
      PathExpr
      LocationStep
      LocationSteps
      NodeTest
      KindTest
      ConstructorExpr
      TagName
      Atom
      NonAtom
      FunctionAppl
      FunctionArgs
      BuiltIns
      LiteralValue
;

Query:           CoreExpr { assert ($$); };

CoreExpr:        Atom;
CoreExpr:        NonAtom;

NonAtom:         BindingExpr;
NonAtom:         TypeswitchExpr;
NonAtom:         SequenceTypeCast;
NonAtom:         SubtypingProof;
NonAtom:         error;
NonAtom:         ConditionalExpr;
NonAtom:         SequenceExpr;
NonAtom:         PathExpr;
NonAtom:         ConstructorExpr;
NonAtom:         FunctionAppl;
NonAtom:         BuiltIns;

BindingExpr:     for_ (var_, nil, LiteralValue, CoreExpr)
    =
    {
        return PFcore_let($1$, $3$, $4$);
    }
    ;
BindingExpr:     for_ (var_, OptVar, LiteralValue, CoreExpr)
    =
    {
        return PFcore_let($1$, 
                          $3$,
                          PFcore_let($2$, 
                                     PFcore_num (1),
                                     $4$));
    }
    ;
BindingExpr:     for_ (var_, OptVar, Atom, CoreExpr);

BindingExpr:     let (var_, Atom, CoreExpr)
    { PHASE (unfold_atoms); }  
    =
    {
        /* Unfold atoms (a atom):
         *
         *     let $v := a return e
         * -->
         *     e[a/$v]
         */
	replace_var ($1$->sem.var, $2$, $3$);
        
      	return $3$; 
    }
    ;
BindingExpr:     let (var_, let (var_, CoreExpr, CoreExpr), CoreExpr)
    { PHASE (unnest_let);

      /* To remove all nested lets (including those that are
       * generated by this rewriting rule), it is necessary
       * to re-rewrite after this transformation has been applied.
       * 
       * This may be expensive (n nested lets lead to the order of n^2
       * repeated rewritings).  This is why we make re-rewriting
       * conditional:
       */
      if (PFstate.optimize >= 3) 
          REWRITE;
    }
    =
    { /* Remove a nested let block:
       * 
       *     let $v1 := let $v2 := e return e' return e''
       * --> 
       *     let $v2 := e return
       *         let $v1 := e' return
       *             e''
       */
      
      PFcnode_t *let_v1;
      PFcnode_t *let_v2;
      
      let_v1 = $$;
      let_v2 = $2$;

      let_v1->child[1] = let_v2->child[2];
      let_v2->child[2] = let_v1;
      
      return let_v2;
    };
BindingExpr:     let (var_, CoreExpr, CoreExpr);

TypeswitchExpr:  typesw (Atom,
                         cases (case_ (SequenceType,
                                       CoreExpr),
                                nil),
                         CoreExpr);

SequenceType:    seqtype;
SequenceType:    stattype (CoreExpr);

SequenceTypeCast: seqcast (SequenceType, CoreExpr);
SubtypingProof:  proof (CoreExpr, SequenceType, CoreExpr);

ConditionalExpr: ifthenelse (Atom, CoreExpr, CoreExpr)
    =
    {
      if (($1$)->kind == c_true)
        return $2$;
      else if (($1$)->kind == c_false)
        return $3$;
    }
    ;

OptVar:          var_;
OptVar:          nil;

SequenceExpr:    seq (Atom, Atom);

PathExpr:        LocationSteps;
PathExpr:        LocationStep;

LocationStep:    ancestor (NodeTest);
LocationStep:    ancestor_or_self (NodeTest);
LocationStep:    attribute (NodeTest);
LocationStep:    child_ (NodeTest);
LocationStep:    descendant (NodeTest);
LocationStep:    descendant_or_self (NodeTest);
LocationStep:    following (NodeTest);
LocationStep:    following_sibling (NodeTest);
LocationStep:    parent_ (NodeTest);
LocationStep:    preceding (NodeTest);
LocationStep:    preceding_sibling (NodeTest);
LocationStep:    self (NodeTest);

LocationSteps:   locsteps (LocationStep, LocationSteps);
LocationSteps:   locsteps (LocationStep, Atom);

NodeTest:        namet;
NodeTest:        KindTest;

KindTest:        kind_node (nil);
KindTest:        kind_comment (nil);
KindTest:        kind_text (nil);
KindTest:        kind_pi (nil);
KindTest:        kind_pi (lit_str);
KindTest:        kind_doc (nil);
KindTest:        kind_elem (nil);
KindTest:        kind_attr (nil);

ConstructorExpr: elem (TagName, CoreExpr);
ConstructorExpr: attr (TagName, CoreExpr);
ConstructorExpr: text (CoreExpr);  
ConstructorExpr: doc (CoreExpr); 
ConstructorExpr: comment (lit_str); 
ConstructorExpr: pi (lit_str);  

TagName:         tag;
TagName:         CoreExpr;

FunctionAppl:    apply (FunctionArgs);

FunctionArgs:    arg (Atom, FunctionArgs);
FunctionArgs:    arg (SequenceTypeCast, FunctionArgs);
FunctionArgs:    nil;


Atom:            var_;
Atom:            LiteralValue;

LiteralValue:    lit_str;
LiteralValue:    lit_int;
LiteralValue:    lit_dec;
LiteralValue:    lit_dbl;
LiteralValue:    true_;
LiteralValue:    false_;
LiteralValue:    empty_;

BuiltIns:        root_;


/* vim:set shiftwidth=4 expandtab filetype=c: */
