/*
** 2001 September 15
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This file contains C code routines that are called by the parser
** to handle INSERT statements in SQLite.
**
** $Id: insert.c,v 1.185 2007/04/18 14:24:33 danielk1977 Exp $
*/
#include "sqliteInt.h"

/*
** Set P3 of the most recently inserted opcode to a column affinity
** string for index pIdx. A column affinity string has one character
** for each column in the table, according to the affinity of the column:
**
**  Character      Column affinity
**  ------------------------------
**  'a'            TEXT
**  'b'            NONE
**  'c'            NUMERIC
**  'd'            INTEGER
**  'e'            REAL
*/
void sqlite3IndexAffinityStr(Vdbe *v, Index *pIdx){
  if( !pIdx->zColAff ){
    /* The first time a column affinity string for a particular index is
    ** required, it is allocated and populated here. It is then stored as
    ** a member of the Index structure for subsequent use.
    **
    ** The column affinity string will eventually be deleted by
    ** sqliteDeleteIndex() when the Index structure itself is cleaned
    ** up.
    */
    int n;
    Table *pTab = pIdx->pTable;
    pIdx->zColAff = (char *)sqliteMalloc(pIdx->nColumn+1);
    if( !pIdx->zColAff ){
      return;
    }
    for(n=0; n<pIdx->nColumn; n++){
      pIdx->zColAff[n] = pTab->aCol[pIdx->aiColumn[n]].affinity;
    }
    pIdx->zColAff[pIdx->nColumn] = '\0';
  }
 
  sqlite3VdbeChangeP3(v, -1, pIdx->zColAff, 0);
}

/*
** Set P3 of the most recently inserted opcode to a column affinity
** string for table pTab. A column affinity string has one character
** for each column indexed by the index, according to the affinity of the
** column:
**
**  Character      Column affinity
**  ------------------------------
**  'a'            TEXT
**  'b'            NONE
**  'c'            NUMERIC
**  'd'            INTEGER
**  'e'            REAL
*/
void sqlite3TableAffinityStr(Vdbe *v, Table *pTab){
  /* The first time a column affinity string for a particular table
  ** is required, it is allocated and populated here. It is then 
  ** stored as a member of the Table structure for subsequent use.
  **
  ** The column affinity string will eventually be deleted by
  ** sqlite3DeleteTable() when the Table structure itself is cleaned up.
  */
  if( !pTab->zColAff ){
    char *zColAff;
    int i;

    zColAff = (char *)sqliteMalloc(pTab->nCol+1);
    if( !zColAff ){
      return;
    }

    for(i=0; i<pTab->nCol; i++){
      zColAff[i] = pTab->aCol[i].affinity;
    }
    zColAff[pTab->nCol] = '\0';

    pTab->zColAff = zColAff;
  }

  sqlite3VdbeChangeP3(v, -1, pTab->zColAff, 0);
}

static int exprReadsTable(Expr *, Schema *, int);
static int selectReadsTable(Select *, Schema *, int);

/*
** Return non-zero if a sub-select somewhere in one of the expressions
** contained in p references table iTab in the database described by 
** pSchema.
*/
static int exprListReadsTable(ExprList *p, Schema *pSchema, int iTab){
  if( p ){
    int ii;
    for(ii=0; ii<p->nExpr; ii++){
      if( exprReadsTable(p->a[ii].pExpr, pSchema, iTab) ){
        return 1;
      }
    }
  }
  return 0;
}

/*
** Return non-zero if a sub-select somewhere in the expression
** p references table iTab in the database described by pSchema.
*/
static int exprReadsTable(Expr *p, Schema *pSchema, int iTab){
  return (
    p && (
      selectReadsTable(p->pSelect, pSchema, iTab) ||
      exprReadsTable(p->pLeft, pSchema, iTab) ||
      exprReadsTable(p->pRight, pSchema, iTab) ||
      exprListReadsTable(p->pList, pSchema, iTab)
    )
  );
}

/*
** Return non-zero if SELECT statement p opens the table with rootpage
** iTab in database iDb.  This is used to see if a statement of the form 
** "INSERT INTO <iDb, iTab> SELECT ..." can run without using temporary
** table for the results of the SELECT. 
**
** The list of tables, views or sub-selects in the FROM clause of the
** select are checked, as are any subqueries that are part of the 
** return-list, WHERE, HAVING or GROUP BY clauses.
*/
static int selectReadsTable(Select *p, Schema *pSchema, int iTab){
  int i;
  struct SrcList_item *pItem;
  if( p==0 ) return 0;
  if( selectReadsTable(p->pPrior, pSchema, iTab) 
   || exprListReadsTable(p->pEList, pSchema, iTab)
   || exprReadsTable(p->pWhere, pSchema, iTab)
   || exprListReadsTable(p->pGroupBy, pSchema, iTab)
   || exprReadsTable(p->pHaving, pSchema, iTab)
  ){
    return 1;
  }
  for(i=0, pItem=p->pSrc->a; i<p->pSrc->nSrc; i++, pItem++){
    if( pItem->pSelect ){
      if( selectReadsTable(pItem->pSelect, pSchema, iTab) ) return 1;
    }else{
      if( pItem->pTab->pSchema==pSchema && pItem->pTab->tnum==iTab ) return 1;
      if( selectReadsTable(pItem->pTab->pSelect, pSchema, iTab) ) return 1;
    }
  }
  return 0;
}

#ifndef SQLITE_OMIT_AUTOINCREMENT
/*
** Write out code to initialize the autoincrement logic.  This code
** looks up the current autoincrement value in the sqlite_sequence
** table and stores that value in a memory cell.  Code generated by
** autoIncStep() will keep that memory cell holding the largest
** rowid value.  Code generated by autoIncEnd() will write the new
** largest value of the counter back into the sqlite_sequence table.
**
** This routine returns the index of the mem[] cell that contains
** the maximum rowid counter.
**
** Two memory cells are allocated.  The next memory cell after the
** one returned holds the rowid in sqlite_sequence where we will
** write back the revised maximum rowid.
*/
static int autoIncBegin(
  Parse *pParse,      /* Parsing context */
  int iDb,            /* Index of the database holding pTab */
  Table *pTab         /* The table we are writing to */
){
  int memId = 0;
  if( pTab->autoInc ){
    Vdbe *v = pParse->pVdbe;
    Db *pDb = &pParse->db->aDb[iDb];
    int iCur = pParse->nTab;
    int addr;
    assert( v );
    addr = sqlite3VdbeCurrentAddr(v);
    memId = pParse->nMem+1;
    pParse->nMem += 2;
    sqlite3OpenTable(pParse, iCur, iDb, pDb->pSchema->pSeqTab, OP_OpenRead);
    sqlite3VdbeAddOp(v, OP_Rewind, iCur, addr+13);
    sqlite3VdbeAddOp(v, OP_Column, iCur, 0);
    sqlite3VdbeOp3(v, OP_String8, 0, 0, pTab->zName, 0);
    sqlite3VdbeAddOp(v, OP_Ne, 0x100, addr+12);
    sqlite3VdbeAddOp(v, OP_Rowid, iCur, 0);
    sqlite3VdbeAddOp(v, OP_MemStore, memId-1, 1);
    sqlite3VdbeAddOp(v, OP_Column, iCur, 1);
    sqlite3VdbeAddOp(v, OP_MemStore, memId, 1);
    sqlite3VdbeAddOp(v, OP_Goto, 0, addr+13);
    sqlite3VdbeAddOp(v, OP_Next, iCur, addr+4);
    sqlite3VdbeAddOp(v, OP_Close, iCur, 0);
  }
  return memId;
}

/*
** Update the maximum rowid for an autoincrement calculation.
**
** This routine should be called when the top of the stack holds a
** new rowid that is about to be inserted.  If that new rowid is
** larger than the maximum rowid in the memId memory cell, then the
** memory cell is updated.  The stack is unchanged.
*/
static void autoIncStep(Parse *pParse, int memId){
  if( memId>0 ){
    sqlite3VdbeAddOp(pParse->pVdbe, OP_MemMax, memId, 0);
  }
}

/*
** After doing one or more inserts, the maximum rowid is stored
** in mem[memId].  Generate code to write this value back into the
** the sqlite_sequence table.
*/
static void autoIncEnd(
  Parse *pParse,     /* The parsing context */
  int iDb,           /* Index of the database holding pTab */
  Table *pTab,       /* Table we are inserting into */
  int memId          /* Memory cell holding the maximum rowid */
){
  if( pTab->autoInc ){
    int iCur = pParse->nTab;
    Vdbe *v = pParse->pVdbe;
    Db *pDb = &pParse->db->aDb[iDb];
    int addr;
    assert( v );
    addr = sqlite3VdbeCurrentAddr(v);
    sqlite3OpenTable(pParse, iCur, iDb, pDb->pSchema->pSeqTab, OP_OpenWrite);
    sqlite3VdbeAddOp(v, OP_MemLoad, memId-1, 0);
    sqlite3VdbeAddOp(v, OP_NotNull, -1, addr+7);
    sqlite3VdbeAddOp(v, OP_Pop, 1, 0);
    sqlite3VdbeAddOp(v, OP_NewRowid, iCur, 0);
    sqlite3VdbeOp3(v, OP_String8, 0, 0, pTab->zName, 0);
    sqlite3VdbeAddOp(v, OP_MemLoad, memId, 0);
    sqlite3VdbeAddOp(v, OP_MakeRecord, 2, 0);
    sqlite3VdbeAddOp(v, OP_Insert, iCur, OPFLAG_APPEND);
    sqlite3VdbeAddOp(v, OP_Close, iCur, 0);
  }
}
#else
/*
** If SQLITE_OMIT_AUTOINCREMENT is defined, then the three routines
** above are all no-ops
*/
# define autoIncBegin(A,B,C) (0)
# define autoIncStep(A,B)
# define autoIncEnd(A,B,C,D)
#endif /* SQLITE_OMIT_AUTOINCREMENT */


/* Forward declaration */
static int xferOptimization(
  Parse *pParse,        /* Parser context */
  Table *pDest,         /* The table we are inserting into */
  Select *pSelect,      /* A SELECT statement to use as the data source */
  int onError,          /* How to handle constraint errors */
  int iDbDest           /* The database of pDest */
);

/*
** This routine is call to handle SQL of the following forms:
**
**    insert into TABLE (IDLIST) values(EXPRLIST)
**    insert into TABLE (IDLIST) select
**
** The IDLIST following the table name is always optional.  If omitted,
** then a list of all columns for the table is substituted.  The IDLIST
** appears in the pColumn parameter.  pColumn is NULL if IDLIST is omitted.
**
** The pList parameter holds EXPRLIST in the first form of the INSERT
** statement above, and pSelect is NULL.  For the second form, pList is
** NULL and pSelect is a pointer to the select statement used to generate
** data for the insert.
**
** The code generated follows one of four templates.  For a simple
** select with data coming from a VALUES clause, the code executes
** once straight down through.  The template looks like this:
**
**         open write cursor to <table> and its indices
**         puts VALUES clause expressions onto the stack
**         write the resulting record into <table>
**         cleanup
**
** The three remaining templates assume the statement is of the form
**
**   INSERT INTO <table> SELECT ...
**
** If the SELECT clause is of the restricted form "SELECT * FROM <table2>" -
** in other words if the SELECT pulls all columns from a single table
** and there is no WHERE or LIMIT or GROUP BY or ORDER BY clauses, and
** if <table2> and <table1> are distinct tables but have identical
** schemas, including all the same indices, then a special optimization
** is invoked that copies raw records from <table2> over to <table1>.
** See the xferOptimization() function for the implementation of this
** template.  This is the second template.
**
**         open a write cursor to <table>
**         open read cursor on <table2>
**         transfer all records in <table2> over to <table>
**         close cursors
**         foreach index on <table>
**           open a write cursor on the <table> index
**           open a read cursor on the corresponding <table2> index
**           transfer all records from the read to the write cursors
**           close cursors
**         end foreach
**
** The third template is for when the second template does not apply
** and the SELECT clause does not read from <table> at any time.
** The generated code follows this template:
**
**         goto B
**      A: setup for the SELECT
**         loop over the rows in the SELECT
**           gosub C
**         end loop
**         cleanup after the SELECT
**         goto D
**      B: open write cursor to <table> and its indices
**         goto A
**      C: insert the select result into <table>
**         return
**      D: cleanup
**
** The fourth template is used if the insert statement takes its
** values from a SELECT but the data is being inserted into a table
** that is also read as part of the SELECT.  In the third form,
** we have to use a intermediate table to store the results of
** the select.  The template is like this:
**
**         goto B
**      A: setup for the SELECT
**         loop over the tables in the SELECT
**           gosub C
**         end loop
**         cleanup after the SELECT
**         goto D
**      C: insert the select result into the intermediate table
**         return
**      B: open a cursor to an intermediate table
**         goto A
**      D: open write cursor to <table> and its indices
**         loop over the intermediate table
**           transfer values form intermediate table into <table>
**         end the loop
**         cleanup
*/
void sqlite3Insert(
  Parse *pParse,        /* Parser context */
  SrcList *pTabList,    /* Name of table into which we are inserting */
  ExprList *pList,      /* List of values to be inserted */
  Select *pSelect,      /* A SELECT statement to use as the data source */
  IdList *pColumn,      /* Column names corresponding to IDLIST. */
  int onError           /* How to handle constraint errors */
){
  Table *pTab;          /* The table to insert into */
  char *zTab;           /* Name of the table into which we are inserting */
  const char *zDb;      /* Name of the database holding this table */
  int i, j, idx;        /* Loop counters */
  Vdbe *v;              /* Generate code into this virtual machine */
  Index *pIdx;          /* For looping over indices of the table */
  int nColumn;          /* Number of columns in the data */
  int base = 0;         /* VDBE Cursor number for pTab */
  int iCont=0,iBreak=0; /* Beginning and end of the loop over srcTab */
  sqlite3 *db;          /* The main database structure */
  int keyColumn = -1;   /* Column that is the INTEGER PRIMARY KEY */
  int endOfLoop;        /* Label for the end of the insertion loop */
  int useTempTable = 0; /* Store SELECT results in intermediate table */
  int srcTab = 0;       /* Data comes from this temporary cursor if >=0 */
  int iSelectLoop = 0;  /* Address of code that implements the SELECT */
  int iCleanup = 0;     /* Address of the cleanup code */
  int iInsertBlock = 0; /* Address of the subroutine used to insert data */
  int iCntMem = 0;      /* Memory cell used for the row counter */
  int newIdx = -1;      /* Cursor for the NEW table */
  Db *pDb;              /* The database containing table being inserted into */
  int counterMem = 0;   /* Memory cell holding AUTOINCREMENT counter */
  int appendFlag = 0;   /* True if the insert is likely to be an append */
  int iDb;

#ifndef SQLITE_OMIT_TRIGGER
  int isView;                 /* True if attempting to insert into a view */
  int triggers_exist = 0;     /* True if there are FOR EACH ROW triggers */
#endif

  if( pParse->nErr || sqlite3MallocFailed() ){
    goto insert_cleanup;
  }
  db = pParse->db;

  /* Locate the table into which we will be inserting new information.
  */
  assert( pTabList->nSrc==1 );
  zTab = pTabList->a[0].zName;
  if( zTab==0 ) goto insert_cleanup;
  pTab = sqlite3SrcListLookup(pParse, pTabList);
  if( pTab==0 ){
    goto insert_cleanup;
  }
  iDb = sqlite3SchemaToIndex(db, pTab->pSchema);
  assert( iDb<db->nDb );
  pDb = &db->aDb[iDb];
  zDb = pDb->zName;
  if( sqlite3AuthCheck(pParse, SQLITE_INSERT, pTab->zName, 0, zDb) ){
    goto insert_cleanup;
  }

  /* Figure out if we have any triggers and if the table being
  ** inserted into is a view
  */
#ifndef SQLITE_OMIT_TRIGGER
  triggers_exist = sqlite3TriggersExist(pParse, pTab, TK_INSERT, 0);
  isView = pTab->pSelect!=0;
#else
# define triggers_exist 0
# define isView 0
#endif
#ifdef SQLITE_OMIT_VIEW
# undef isView
# define isView 0
#endif

  /* Ensure that:
  *  (a) the table is not read-only, 
  *  (b) that if it is a view then ON INSERT triggers exist
  */
  if( sqlite3IsReadOnly(pParse, pTab, triggers_exist) ){
    goto insert_cleanup;
  }
  assert( pTab!=0 );

  /* If pTab is really a view, make sure it has been initialized.
  ** ViewGetColumnNames() is a no-op if pTab is not a view (or virtual 
  ** module table).
  */
  if( sqlite3ViewGetColumnNames(pParse, pTab) ){
    goto insert_cleanup;
  }

  /* Allocate a VDBE
  */
  v = sqlite3GetVdbe(pParse);
  if( v==0 ) goto insert_cleanup;
  if( pParse->nested==0 ) sqlite3VdbeCountChanges(v);
  sqlite3BeginWriteOperation(pParse, pSelect || triggers_exist, iDb);

  /* if there are row triggers, allocate a temp table for new.* references. */
  if( triggers_exist ){
    newIdx = pParse->nTab++;
  }

#ifndef SQLITE_OMIT_XFER_OPT
  /* If the statement is of the form
  **
  **       INSERT INTO <table1> SELECT * FROM <table2>;
  **
  ** Then special optimizations can be applied that make the transfer
  ** very fast and which reduce fragmentation of indices.
  */
  if( pColumn==0 && xferOptimization(pParse, pTab, pSelect, onError, iDb) ){
    assert( !triggers_exist );
    assert( pList==0 );
    goto insert_cleanup;
  }
#endif /* SQLITE_OMIT_XFER_OPT */

  /* If this is an AUTOINCREMENT table, look up the sequence number in the
  ** sqlite_sequence table and store it in memory cell counterMem.  Also
  ** remember the rowid of the sqlite_sequence table entry in memory cell
  ** counterRowid.
  */
  counterMem = autoIncBegin(pParse, iDb, pTab);

  /* Figure out how many columns of data are supplied.  If the data
  ** is coming from a SELECT statement, then this step also generates
  ** all the code to implement the SELECT statement and invoke a subroutine
  ** to process each row of the result. (Template 2.) If the SELECT
  ** statement uses the the table that is being inserted into, then the
  ** subroutine is also coded here.  That subroutine stores the SELECT
  ** results in a temporary table. (Template 3.)
  */
  if( pSelect ){
    /* Data is coming from a SELECT.  Generate code to implement that SELECT
    */
    int rc, iInitCode;
    iInitCode = sqlite3VdbeAddOp(v, OP_Goto, 0, 0);
    iSelectLoop = sqlite3VdbeCurrentAddr(v);
    iInsertBlock = sqlite3VdbeMakeLabel(v);

    /* Resolve the expressions in the SELECT statement and execute it. */
    rc = sqlite3Select(pParse, pSelect, SRT_Subroutine, iInsertBlock,0,0,0,0);
    if( rc || pParse->nErr || sqlite3MallocFailed() ){
      goto insert_cleanup;
    }

    iCleanup = sqlite3VdbeMakeLabel(v);
    sqlite3VdbeAddOp(v, OP_Goto, 0, iCleanup);
    assert( pSelect->pEList );
    nColumn = pSelect->pEList->nExpr;

    /* Set useTempTable to TRUE if the result of the SELECT statement
    ** should be written into a temporary table.  Set to FALSE if each
    ** row of the SELECT can be written directly into the result table.
    **
    ** A temp table must be used if the table being updated is also one
    ** of the tables being read by the SELECT statement.  Also use a 
    ** temp table in the case of row triggers.
    */
    if( triggers_exist || selectReadsTable(pSelect,pTab->pSchema,pTab->tnum) ){
      useTempTable = 1;
    }

    if( useTempTable ){
      /* Generate the subroutine that SELECT calls to process each row of
      ** the result.  Store the result in a temporary table
      */
      srcTab = pParse->nTab++;
      sqlite3VdbeResolveLabel(v, iInsertBlock);
      sqlite3VdbeAddOp(v, OP_StackDepth, -1, 0);
      sqlite3VdbeAddOp(v, OP_MakeRecord, nColumn, 0);
      sqlite3VdbeAddOp(v, OP_NewRowid, srcTab, 0);
      sqlite3VdbeAddOp(v, OP_Pull, 1, 0);
      sqlite3VdbeAddOp(v, OP_Insert, srcTab, OPFLAG_APPEND);
      sqlite3VdbeAddOp(v, OP_Return, 0, 0);

      /* The following code runs first because the GOTO at the very top
      ** of the program jumps to it.  Create the temporary table, then jump
      ** back up and execute the SELECT code above.
      */
      sqlite3VdbeJumpHere(v, iInitCode);
      sqlite3VdbeAddOp(v, OP_OpenEphemeral, srcTab, 0);
      sqlite3VdbeAddOp(v, OP_SetNumColumns, srcTab, nColumn);
      sqlite3VdbeAddOp(v, OP_Goto, 0, iSelectLoop);
      sqlite3VdbeResolveLabel(v, iCleanup);
    }else{
      sqlite3VdbeJumpHere(v, iInitCode);
    }
  }else{
    /* This is the case if the data for the INSERT is coming from a VALUES
    ** clause
    */
    NameContext sNC;
    assert(useTempTable==0);
    memset(&sNC, 0, sizeof(sNC));
    sNC.pParse = pParse;
    srcTab = -1;
    nColumn = pList ? pList->nExpr : 0;
    for(i=0; i<nColumn; i++){
      if( sqlite3ExprResolveNames(&sNC, pList->a[i].pExpr) ){
        goto insert_cleanup;
      }
    }
  }

  /* Make sure the number of columns in the source data matches the number
  ** of columns to be inserted into the table.
  */
  if( pColumn==0 && nColumn && nColumn!=pTab->nCol ){
    sqlite3ErrorMsg(pParse, 
       "table %S has %d columns but %d values were supplied",
       pTabList, 0, pTab->nCol, nColumn);
    goto insert_cleanup;
  }
  if( pColumn!=0 && nColumn!=pColumn->nId ){
    sqlite3ErrorMsg(pParse, "%d values for %d columns", nColumn, pColumn->nId);
    goto insert_cleanup;
  }

  /* If the INSERT statement included an IDLIST term, then make sure
  ** all elements of the IDLIST really are columns of the table and 
  ** remember the column indices.
  **
  ** If the table has an INTEGER PRIMARY KEY column and that column
  ** is named in the IDLIST, then record in the keyColumn variable
  ** the index into IDLIST of the primary key column.  keyColumn is
  ** the index of the primary key as it appears in IDLIST, not as
  ** is appears in the original table.  (The index of the primary
  ** key in the original table is pTab->iPKey.)
  */
  if( pColumn ){
    for(i=0; i<pColumn->nId; i++){
      pColumn->a[i].idx = -1;
    }
    for(i=0; i<pColumn->nId; i++){
      for(j=0; j<pTab->nCol; j++){
        if( sqlite3StrICmp(pColumn->a[i].zName, pTab->aCol[j].zName)==0 ){
          pColumn->a[i].idx = j;
          if( j==pTab->iPKey ){
            keyColumn = i;
          }
          break;
        }
      }
      if( j>=pTab->nCol ){
        if( sqlite3IsRowid(pColumn->a[i].zName) ){
          keyColumn = i;
        }else{
          sqlite3ErrorMsg(pParse, "table %S has no column named %s",
              pTabList, 0, pColumn->a[i].zName);
          pParse->nErr++;
          goto insert_cleanup;
        }
      }
    }
  }

  /* If there is no IDLIST term but the table has an integer primary
  ** key, the set the keyColumn variable to the primary key column index
  ** in the original table definition.
  */
  if( pColumn==0 && nColumn>0 ){
    keyColumn = pTab->iPKey;
  }

  /* Open the temp table for FOR EACH ROW triggers
  */
  if( triggers_exist ){
    sqlite3VdbeAddOp(v, OP_OpenPseudo, newIdx, 0);
    sqlite3VdbeAddOp(v, OP_SetNumColumns, newIdx, pTab->nCol);
  }
    
  /* Initialize the count of rows to be inserted
  */
  if( db->flags & SQLITE_CountRows ){
    iCntMem = pParse->nMem++;
    sqlite3VdbeAddOp(v, OP_MemInt, 0, iCntMem);
  }

  /* Open tables and indices if there are no row triggers */
  if( !triggers_exist ){
    base = pParse->nTab;
    sqlite3OpenTableAndIndices(pParse, pTab, base, OP_OpenWrite);
  }

  /* If the data source is a temporary table, then we have to create
  ** a loop because there might be multiple rows of data.  If the data
  ** source is a subroutine call from the SELECT statement, then we need
  ** to launch the SELECT statement processing.
  */
  if( useTempTable ){
    iBreak = sqlite3VdbeMakeLabel(v);
    sqlite3VdbeAddOp(v, OP_Rewind, srcTab, iBreak);
    iCont = sqlite3VdbeCurrentAddr(v);
  }else if( pSelect ){
    sqlite3VdbeAddOp(v, OP_Goto, 0, iSelectLoop);
    sqlite3VdbeResolveLabel(v, iInsertBlock);
    sqlite3VdbeAddOp(v, OP_StackDepth, -1, 0);
  }

  /* Run the BEFORE and INSTEAD OF triggers, if there are any
  */
  endOfLoop = sqlite3VdbeMakeLabel(v);
  if( triggers_exist & TRIGGER_BEFORE ){

    /* build the NEW.* reference row.  Note that if there is an INTEGER
    ** PRIMARY KEY into which a NULL is being inserted, that NULL will be
    ** translated into a unique ID for the row.  But on a BEFORE trigger,
    ** we do not know what the unique ID will be (because the insert has
    ** not happened yet) so we substitute a rowid of -1
    */
    if( keyColumn<0 ){
      sqlite3VdbeAddOp(v, OP_Integer, -1, 0);
    }else if( useTempTable ){
      sqlite3VdbeAddOp(v, OP_Column, srcTab, keyColumn);
    }else{
      assert( pSelect==0 );  /* Otherwise useTempTable is true */
      sqlite3ExprCode(pParse, pList->a[keyColumn].pExpr);
      sqlite3VdbeAddOp(v, OP_NotNull, -1, sqlite3VdbeCurrentAddr(v)+3);
      sqlite3VdbeAddOp(v, OP_Pop, 1, 0);
      sqlite3VdbeAddOp(v, OP_Integer, -1, 0);
      sqlite3VdbeAddOp(v, OP_MustBeInt, 0, 0);
    }

    /* Create the new column data
    */
    for(i=0; i<pTab->nCol; i++){
      if( pColumn==0 ){
        j = i;
      }else{
        for(j=0; j<pColumn->nId; j++){
          if( pColumn->a[j].idx==i ) break;
        }
      }
      if( pColumn && j>=pColumn->nId ){
        sqlite3ExprCode(pParse, pTab->aCol[i].pDflt);
      }else if( useTempTable ){
        sqlite3VdbeAddOp(v, OP_Column, srcTab, j); 
      }else{
        assert( pSelect==0 ); /* Otherwise useTempTable is true */
        sqlite3ExprCodeAndCache(pParse, pList->a[j].pExpr);
      }
    }
    sqlite3VdbeAddOp(v, OP_MakeRecord, pTab->nCol, 0);

    /* If this is an INSERT on a view with an INSTEAD OF INSERT trigger,
    ** do not attempt any conversions before assembling the record.
    ** If this is a real table, attempt conversions as required by the
    ** table column affinities.
    */
    if( !isView ){
      sqlite3TableAffinityStr(v, pTab);
    }
    sqlite3VdbeAddOp(v, OP_Insert, newIdx, 0);

    /* Fire BEFORE or INSTEAD OF triggers */
    if( sqlite3CodeRowTrigger(pParse, TK_INSERT, 0, TRIGGER_BEFORE, pTab, 
        newIdx, -1, onError, endOfLoop) ){
      goto insert_cleanup;
    }
  }

  /* If any triggers exists, the opening of tables and indices is deferred
  ** until now.
  */
  if( triggers_exist && !isView ){
    base = pParse->nTab;
    sqlite3OpenTableAndIndices(pParse, pTab, base, OP_OpenWrite);
  }

  /* Push the record number for the new entry onto the stack.  The
  ** record number is a randomly generate integer created by NewRowid
  ** except when the table has an INTEGER PRIMARY KEY column, in which
  ** case the record number is the same as that column. 
  */
  if( !isView ){
    if( IsVirtual(pTab) ){
      /* The row that the VUpdate opcode will delete:  none */
      sqlite3VdbeAddOp(v, OP_Null, 0, 0);
    }
    if( keyColumn>=0 ){
      if( useTempTable ){
        sqlite3VdbeAddOp(v, OP_Column, srcTab, keyColumn);
      }else if( pSelect ){
        sqlite3VdbeAddOp(v, OP_Dup, nColumn - keyColumn - 1, 1);
      }else{
        VdbeOp *pOp;
        sqlite3ExprCode(pParse, pList->a[keyColumn].pExpr);
        pOp = sqlite3VdbeGetOp(v, sqlite3VdbeCurrentAddr(v) - 1);
        if( pOp && pOp->opcode==OP_Null ){
          appendFlag = 1;
          pOp->opcode = OP_NewRowid;
          pOp->p1 = base;
          pOp->p2 = counterMem;
        }
      }
      /* If the PRIMARY KEY expression is NULL, then use OP_NewRowid
      ** to generate a unique primary key value.
      */
      if( !appendFlag ){
        sqlite3VdbeAddOp(v, OP_NotNull, -1, sqlite3VdbeCurrentAddr(v)+3);
        sqlite3VdbeAddOp(v, OP_Pop, 1, 0);
        sqlite3VdbeAddOp(v, OP_NewRowid, base, counterMem);
        sqlite3VdbeAddOp(v, OP_MustBeInt, 0, 0);
      }
    }else if( IsVirtual(pTab) ){
      sqlite3VdbeAddOp(v, OP_Null, 0, 0);
    }else{
      sqlite3VdbeAddOp(v, OP_NewRowid, base, counterMem);
      appendFlag = 1;
    }
    autoIncStep(pParse, counterMem);

    /* Push onto the stack, data for all columns of the new entry, beginning
    ** with the first column.
    */
    for(i=0; i<pTab->nCol; i++){
      if( i==pTab->iPKey ){
        /* The value of the INTEGER PRIMARY KEY column is always a NULL.
        ** Whenever this column is read, the record number will be substituted
        ** in its place.  So will fill this column with a NULL to avoid
        ** taking up data space with information that will never be used. */
        sqlite3VdbeAddOp(v, OP_Null, 0, 0);
        continue;
      }
      if( pColumn==0 ){
        j = i;
      }else{
        for(j=0; j<pColumn->nId; j++){
          if( pColumn->a[j].idx==i ) break;
        }
      }
      if( nColumn==0 || (pColumn && j>=pColumn->nId) ){
        sqlite3ExprCode(pParse, pTab->aCol[i].pDflt);
      }else if( useTempTable ){
        sqlite3VdbeAddOp(v, OP_Column, srcTab, j); 
      }else if( pSelect ){
        sqlite3VdbeAddOp(v, OP_Dup, i+nColumn-j+IsVirtual(pTab), 1);
      }else{
        sqlite3ExprCode(pParse, pList->a[j].pExpr);
      }
    }

    /* Generate code to check constraints and generate index keys and
    ** do the insertion.
    */
#ifndef SQLITE_OMIT_VIRTUALTABLE
    if( IsVirtual(pTab) ){
      pParse->pVirtualLock = pTab;
      sqlite3VdbeOp3(v, OP_VUpdate, 1, pTab->nCol+2,
                     (const char*)pTab->pVtab, P3_VTAB);
    }else
#endif
    {
      sqlite3GenerateConstraintChecks(pParse, pTab, base, 0, keyColumn>=0,
                                     0, onError, endOfLoop);
      sqlite3CompleteInsertion(pParse, pTab, base, 0,0,0,
                            (triggers_exist & TRIGGER_AFTER)!=0 ? newIdx : -1,
                            appendFlag);
    }
  }

  /* Update the count of rows that are inserted
  */
  if( (db->flags & SQLITE_CountRows)!=0 ){
    sqlite3VdbeAddOp(v, OP_MemIncr, 1, iCntMem);
  }

  if( triggers_exist ){
    /* Close all tables opened */
    if( !isView ){
      sqlite3VdbeAddOp(v, OP_Close, base, 0);
      for(idx=1, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext, idx++){
        sqlite3VdbeAddOp(v, OP_Close, idx+base, 0);
      }
    }

    /* Code AFTER triggers */
    if( sqlite3CodeRowTrigger(pParse, TK_INSERT, 0, TRIGGER_AFTER, pTab,
          newIdx, -1, onError, endOfLoop) ){
      goto insert_cleanup;
    }
  }

  /* The bottom of the loop, if the data source is a SELECT statement
  */
  sqlite3VdbeResolveLabel(v, endOfLoop);
  if( useTempTable ){
    sqlite3VdbeAddOp(v, OP_Next, srcTab, iCont);
    sqlite3VdbeResolveLabel(v, iBreak);
    sqlite3VdbeAddOp(v, OP_Close, srcTab, 0);
  }else if( pSelect ){
    sqlite3VdbeAddOp(v, OP_Pop, nColumn, 0);
    sqlite3VdbeAddOp(v, OP_Return, 0, 0);
    sqlite3VdbeResolveLabel(v, iCleanup);
  }

  if( !triggers_exist && !IsVirtual(pTab) ){
    /* Close all tables opened */
    sqlite3VdbeAddOp(v, OP_Close, base, 0);
    for(idx=1, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext, idx++){
      sqlite3VdbeAddOp(v, OP_Close, idx+base, 0);
    }
  }

  /* Update the sqlite_sequence table by storing the content of the
  ** counter value in memory counterMem back into the sqlite_sequence
  ** table.
  */
  autoIncEnd(pParse, iDb, pTab, counterMem);

  /*
  ** Return the number of rows inserted. If this routine is 
  ** generating code because of a call to sqlite3NestedParse(), do not
  ** invoke the callback function.
  */
  if( db->flags & SQLITE_CountRows && pParse->nested==0 && !pParse->trigStack ){
    sqlite3VdbeAddOp(v, OP_MemLoad, iCntMem, 0);
    sqlite3VdbeAddOp(v, OP_Callback, 1, 0);
    sqlite3VdbeSetNumCols(v, 1);
    sqlite3VdbeSetColName(v, 0, COLNAME_NAME, "rows inserted", P3_STATIC);
  }

insert_cleanup:
  sqlite3SrcListDelete(pTabList);
  sqlite3ExprListDelete(pList);
  sqlite3SelectDelete(pSelect);
  sqlite3IdListDelete(pColumn);
}

/*
** Generate code to do a constraint check prior to an INSERT or an UPDATE.
**
** When this routine is called, the stack contains (from bottom to top)
** the following values:
**
**    1.  The rowid of the row to be updated before the update.  This
**        value is omitted unless we are doing an UPDATE that involves a
**        change to the record number.
**
**    2.  The rowid of the row after the update.
**
**    3.  The data in the first column of the entry after the update.
**
**    i.  Data from middle columns...
**
**    N.  The data in the last column of the entry after the update.
**
** The old rowid shown as entry (1) above is omitted unless both isUpdate
** and rowidChng are 1.  isUpdate is true for UPDATEs and false for
** INSERTs and rowidChng is true if the record number is being changed.
**
** The code generated by this routine pushes additional entries onto
** the stack which are the keys for new index entries for the new record.
** The order of index keys is the same as the order of the indices on
** the pTable->pIndex list.  A key is only created for index i if 
** aIdxUsed!=0 and aIdxUsed[i]!=0.
**
** This routine also generates code to check constraints.  NOT NULL,
** CHECK, and UNIQUE constraints are all checked.  If a constraint fails,
** then the appropriate action is performed.  There are five possible
** actions: ROLLBACK, ABORT, FAIL, REPLACE, and IGNORE.
**
**  Constraint type  Action       What Happens
**  ---------------  ----------   ----------------------------------------
**  any              ROLLBACK     The current transaction is rolled back and
**                                sqlite3_exec() returns immediately with a
**                                return code of SQLITE_CONSTRAINT.
**
**  any              ABORT        Back out changes from the current command
**                                only (do not do a complete rollback) then
**                                cause sqlite3_exec() to return immediately
**                                with SQLITE_CONSTRAINT.
**
**  any              FAIL         Sqlite_exec() returns immediately with a
**                                return code of SQLITE_CONSTRAINT.  The
**                                transaction is not rolled back and any
**                                prior changes are retained.
**
**  any              IGNORE       The record number and data is popped from
**                                the stack and there is an immediate jump
**                                to label ignoreDest.
**
**  NOT NULL         REPLACE      The NULL value is replace by the default
**                                value for that column.  If the default value
**                                is NULL, the action is the same as ABORT.
**
**  UNIQUE           REPLACE      The other row that conflicts with the row
**                                being inserted is removed.
**
**  CHECK            REPLACE      Illegal.  The results in an exception.
**
** Which action to take is determined by the overrideError parameter.
** Or if overrideError==OE_Default, then the pParse->onError parameter
** is used.  Or if pParse->onError==OE_Default then the onError value
** for the constraint is used.
**
** The calling routine must open a read/write cursor for pTab with
** cursor number "base".  All indices of pTab must also have open
** read/write cursors with cursor number base+i for the i-th cursor.
** Except, if there is no possibility of a REPLACE action then
** cursors do not need to be open for indices where aIdxUsed[i]==0.
**
** If the isUpdate flag is true, it means that the "base" cursor is
** initially pointing to an entry that is being updated.  The isUpdate
** flag causes extra code to be generated so that the "base" cursor
** is still pointing at the same entry after the routine returns.
** Without the isUpdate flag, the "base" cursor might be moved.
*/
void sqlite3GenerateConstraintChecks(
  Parse *pParse,      /* The parser context */
  Table *pTab,        /* the table into which we are inserting */
  int base,           /* Index of a read/write cursor pointing at pTab */
  char *aIdxUsed,     /* Which indices are used.  NULL means all are used */
  int rowidChng,      /* True if the record number will change */
  int isUpdate,       /* True for UPDATE, False for INSERT */
  int overrideError,  /* Override onError to this if not OE_Default */
  int ignoreDest      /* Jump to this label on an OE_Ignore resolution */
){
  int i;
  Vdbe *v;
  int nCol;
  int onError;
  int addr;
  int extra;
  int iCur;
  Index *pIdx;
  int seenReplace = 0;
  int jumpInst1=0, jumpInst2;
  int hasTwoRowids = (isUpdate && rowidChng);

  v = sqlite3GetVdbe(pParse);
  assert( v!=0 );
  assert( pTab->pSelect==0 );  /* This table is not a VIEW */
  nCol = pTab->nCol;

  /* Test all NOT NULL constraints.
  */
  for(i=0; i<nCol; i++){
    if( i==pTab->iPKey ){
      continue;
    }
    onError = pTab->aCol[i].notNull;
    if( onError==OE_None ) continue;
    if( overrideError!=OE_Default ){
      onError = overrideError;
    }else if( onError==OE_Default ){
      onError = OE_Abort;
    }
    if( onError==OE_Replace && pTab->aCol[i].pDflt==0 ){
      onError = OE_Abort;
    }
    sqlite3VdbeAddOp(v, OP_Dup, nCol-1-i, 1);
    addr = sqlite3VdbeAddOp(v, OP_NotNull, 1, 0);
    assert( onError==OE_Rollback || onError==OE_Abort || onError==OE_Fail
        || onError==OE_Ignore || onError==OE_Replace );
    switch( onError ){
      case OE_Rollback:
      case OE_Abort:
      case OE_Fail: {
        char *zMsg = 0;
        sqlite3VdbeAddOp(v, OP_Halt, SQLITE_CONSTRAINT, onError);
        sqlite3SetString(&zMsg, pTab->zName, ".", pTab->aCol[i].zName,
                        " may not be NULL", (char*)0);
        sqlite3VdbeChangeP3(v, -1, zMsg, P3_DYNAMIC);
        break;
      }
      case OE_Ignore: {
        sqlite3VdbeAddOp(v, OP_Pop, nCol+1+hasTwoRowids, 0);
        sqlite3VdbeAddOp(v, OP_Goto, 0, ignoreDest);
        break;
      }
      case OE_Replace: {
        sqlite3ExprCode(pParse, pTab->aCol[i].pDflt);
        sqlite3VdbeAddOp(v, OP_Push, nCol-i, 0);
        break;
      }
    }
    sqlite3VdbeJumpHere(v, addr);
  }

  /* Test all CHECK constraints
  */
#ifndef SQLITE_OMIT_CHECK
  if( pTab->pCheck && (pParse->db->flags & SQLITE_IgnoreChecks)==0 ){
    int allOk = sqlite3VdbeMakeLabel(v);
    assert( pParse->ckOffset==0 );
    pParse->ckOffset = nCol;
    sqlite3ExprIfTrue(pParse, pTab->pCheck, allOk, 1);
    assert( pParse->ckOffset==nCol );
    pParse->ckOffset = 0;
    onError = overrideError!=OE_Default ? overrideError : OE_Abort;
    if( onError==OE_Ignore || onError==OE_Replace ){
      sqlite3VdbeAddOp(v, OP_Pop, nCol+1+hasTwoRowids, 0);
      sqlite3VdbeAddOp(v, OP_Goto, 0, ignoreDest);
    }else{
      sqlite3VdbeAddOp(v, OP_Halt, SQLITE_CONSTRAINT, onError);
    }
    sqlite3VdbeResolveLabel(v, allOk);
  }
#endif /* !defined(SQLITE_OMIT_CHECK) */

  /* If we have an INTEGER PRIMARY KEY, make sure the primary key
  ** of the new record does not previously exist.  Except, if this
  ** is an UPDATE and the primary key is not changing, that is OK.
  */
  if( rowidChng ){
    onError = pTab->keyConf;
    if( overrideError!=OE_Default ){
      onError = overrideError;
    }else if( onError==OE_Default ){
      onError = OE_Abort;
    }
    
    if( isUpdate ){
      sqlite3VdbeAddOp(v, OP_Dup, nCol+1, 1);
      sqlite3VdbeAddOp(v, OP_Dup, nCol+1, 1);
      jumpInst1 = sqlite3VdbeAddOp(v, OP_Eq, 0, 0);
    }
    sqlite3VdbeAddOp(v, OP_Dup, nCol, 1);
    jumpInst2 = sqlite3VdbeAddOp(v, OP_NotExists, base, 0);
    switch( onError ){
      default: {
        onError = OE_Abort;
        /* Fall thru into the next case */
      }
      case OE_Rollback:
      case OE_Abort:
      case OE_Fail: {
        sqlite3VdbeOp3(v, OP_Halt, SQLITE_CONSTRAINT, onError,
                         "PRIMARY KEY must be unique", P3_STATIC);
        break;
      }
      case OE_Replace: {
        sqlite3GenerateRowIndexDelete(v, pTab, base, 0);
        if( isUpdate ){
          sqlite3VdbeAddOp(v, OP_Dup, nCol+hasTwoRowids, 1);
          sqlite3VdbeAddOp(v, OP_MoveGe, base, 0);
        }
        seenReplace = 1;
        break;
      }
      case OE_Ignore: {
        assert( seenReplace==0 );
        sqlite3VdbeAddOp(v, OP_Pop, nCol+1+hasTwoRowids, 0);
        sqlite3VdbeAddOp(v, OP_Goto, 0, ignoreDest);
        break;
      }
    }
    sqlite3VdbeJumpHere(v, jumpInst2);
    if( isUpdate ){
      sqlite3VdbeJumpHere(v, jumpInst1);
      sqlite3VdbeAddOp(v, OP_Dup, nCol+1, 1);
      sqlite3VdbeAddOp(v, OP_MoveGe, base, 0);
    }
  }

  /* Test all UNIQUE constraints by creating entries for each UNIQUE
  ** index and making sure that duplicate entries do not already exist.
  ** Add the new records to the indices as we go.
  */
  extra = -1;
  for(iCur=0, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext, iCur++){
    if( aIdxUsed && aIdxUsed[iCur]==0 ) continue;  /* Skip unused indices */
    extra++;

    /* Create a key for accessing the index entry */
    sqlite3VdbeAddOp(v, OP_Dup, nCol+extra, 1);
    for(i=0; i<pIdx->nColumn; i++){
      int idx = pIdx->aiColumn[i];
      if( idx==pTab->iPKey ){
        sqlite3VdbeAddOp(v, OP_Dup, i+extra+nCol+1, 1);
      }else{
        sqlite3VdbeAddOp(v, OP_Dup, i+extra+nCol-idx, 1);
      }
    }
    jumpInst1 = sqlite3VdbeAddOp(v, OP_MakeIdxRec, pIdx->nColumn, 0);
    sqlite3IndexAffinityStr(v, pIdx);

    /* Find out what action to take in case there is an indexing conflict */
    onError = pIdx->onError;
    if( onError==OE_None ) continue;  /* pIdx is not a UNIQUE index */
    if( overrideError!=OE_Default ){
      onError = overrideError;
    }else if( onError==OE_Default ){
      onError = OE_Abort;
    }
    if( seenReplace ){
      if( onError==OE_Ignore ) onError = OE_Replace;
      else if( onError==OE_Fail ) onError = OE_Abort;
    }
    

    /* Check to see if the new index entry will be unique */
    sqlite3VdbeAddOp(v, OP_Dup, extra+nCol+1+hasTwoRowids, 1);
    jumpInst2 = sqlite3VdbeAddOp(v, OP_IsUnique, base+iCur+1, 0);

    /* Generate code that executes if the new index entry is not unique */
    assert( onError==OE_Rollback || onError==OE_Abort || onError==OE_Fail
        || onError==OE_Ignore || onError==OE_Replace );
    switch( onError ){
      case OE_Rollback:
      case OE_Abort:
      case OE_Fail: {
        int j, n1, n2;
        char zErrMsg[200];
        strcpy(zErrMsg, pIdx->nColumn>1 ? "columns " : "column ");
        n1 = strlen(zErrMsg);
        for(j=0; j<pIdx->nColumn && n1<sizeof(zErrMsg)-30; j++){
          char *zCol = pTab->aCol[pIdx->aiColumn[j]].zName;
          n2 = strlen(zCol);
          if( j>0 ){
            strcpy(&zErrMsg[n1], ", ");
            n1 += 2;
          }
          if( n1+n2>sizeof(zErrMsg)-30 ){
            strcpy(&zErrMsg[n1], "...");
            n1 += 3;
            break;
          }else{
            strcpy(&zErrMsg[n1], zCol);
            n1 += n2;
          }
        }
        strcpy(&zErrMsg[n1], 
            pIdx->nColumn>1 ? " are not unique" : " is not unique");
        sqlite3VdbeOp3(v, OP_Halt, SQLITE_CONSTRAINT, onError, zErrMsg, 0);
        break;
      }
      case OE_Ignore: {
        assert( seenReplace==0 );
        sqlite3VdbeAddOp(v, OP_Pop, nCol+extra+3+hasTwoRowids, 0);
        sqlite3VdbeAddOp(v, OP_Goto, 0, ignoreDest);
        break;
      }
      case OE_Replace: {
        sqlite3GenerateRowDelete(pParse->db, v, pTab, base, 0);
        if( isUpdate ){
          sqlite3VdbeAddOp(v, OP_Dup, nCol+extra+1+hasTwoRowids, 1);
          sqlite3VdbeAddOp(v, OP_MoveGe, base, 0);
        }
        seenReplace = 1;
        break;
      }
    }
#if NULL_DISTINCT_FOR_UNIQUE
    sqlite3VdbeJumpHere(v, jumpInst1);
#endif
    sqlite3VdbeJumpHere(v, jumpInst2);
  }
}

/*
** This routine generates code to finish the INSERT or UPDATE operation
** that was started by a prior call to sqlite3GenerateConstraintChecks.
** The stack must contain keys for all active indices followed by data
** and the rowid for the new entry.  This routine creates the new
** entries in all indices and in the main table.
**
** The arguments to this routine should be the same as the first six
** arguments to sqlite3GenerateConstraintChecks.
*/
void sqlite3CompleteInsertion(
  Parse *pParse,      /* The parser context */
  Table *pTab,        /* the table into which we are inserting */
  int base,           /* Index of a read/write cursor pointing at pTab */
  char *aIdxUsed,     /* Which indices are used.  NULL means all are used */
  int rowidChng,      /* True if the record number will change */
  int isUpdate,       /* True for UPDATE, False for INSERT */
  int newIdx,         /* Index of NEW table for triggers.  -1 if none */
  int appendBias      /* True if this is likely to be an append */
){
  int i;
  Vdbe *v;
  int nIdx;
  Index *pIdx;
  int pik_flags;

  v = sqlite3GetVdbe(pParse);
  assert( v!=0 );
  assert( pTab->pSelect==0 );  /* This table is not a VIEW */
  for(nIdx=0, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext, nIdx++){}
  for(i=nIdx-1; i>=0; i--){
    if( aIdxUsed && aIdxUsed[i]==0 ) continue;
    sqlite3VdbeAddOp(v, OP_IdxInsert, base+i+1, 0);
  }
  sqlite3VdbeAddOp(v, OP_MakeRecord, pTab->nCol, 0);
  sqlite3TableAffinityStr(v, pTab);
#ifndef SQLITE_OMIT_TRIGGER
  if( newIdx>=0 ){
    sqlite3VdbeAddOp(v, OP_Dup, 1, 0);
    sqlite3VdbeAddOp(v, OP_Dup, 1, 0);
    sqlite3VdbeAddOp(v, OP_Insert, newIdx, 0);
  }
#endif
  if( pParse->nested ){
    pik_flags = 0;
  }else{
    pik_flags = OPFLAG_NCHANGE;
    pik_flags |= (isUpdate?OPFLAG_ISUPDATE:OPFLAG_LASTROWID);
  }
  if( appendBias ){
    pik_flags |= OPFLAG_APPEND;
  }
  sqlite3VdbeAddOp(v, OP_Insert, base, pik_flags);
  if( !pParse->nested ){
    sqlite3VdbeChangeP3(v, -1, pTab->zName, P3_STATIC);
  }
  
  if( isUpdate && rowidChng ){
    sqlite3VdbeAddOp(v, OP_Pop, 1, 0);
  }
}

/*
** Generate code that will open cursors for a table and for all
** indices of that table.  The "base" parameter is the cursor number used
** for the table.  Indices are opened on subsequent cursors.
*/
void sqlite3OpenTableAndIndices(
  Parse *pParse,   /* Parsing context */
  Table *pTab,     /* Table to be opened */
  int base,        /* Cursor number assigned to the table */
  int op           /* OP_OpenRead or OP_OpenWrite */
){
  int i;
  int iDb;
  Index *pIdx;
  Vdbe *v;

  if( IsVirtual(pTab) ) return;
  iDb = sqlite3SchemaToIndex(pParse->db, pTab->pSchema);
  v = sqlite3GetVdbe(pParse);
  assert( v!=0 );
  sqlite3OpenTable(pParse, base, iDb, pTab, op);
  for(i=1, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext, i++){
    KeyInfo *pKey = sqlite3IndexKeyinfo(pParse, pIdx);
    assert( pIdx->pSchema==pTab->pSchema );
    sqlite3VdbeAddOp(v, OP_Integer, iDb, 0);
    VdbeComment((v, "# %s", pIdx->zName));
    sqlite3VdbeOp3(v, op, i+base, pIdx->tnum, (char*)pKey, P3_KEYINFO_HANDOFF);
  }
  if( pParse->nTab<=base+i ){
    pParse->nTab = base+i;
  }
}


#ifdef SQLITE_TEST
/*
** The following global variable is incremented whenever the
** transfer optimization is used.  This is used for testing
** purposes only - to make sure the transfer optimization really
** is happening when it is suppose to.
*/
int sqlite3_xferopt_count;
#endif /* SQLITE_TEST */


#ifndef SQLITE_OMIT_XFER_OPT
/*
** Check to collation names to see if they are compatible.
*/
static int xferCompatibleCollation(const char *z1, const char *z2){
  if( z1==0 ){
    return z2==0;
  }
  if( z2==0 ){
    return 0;
  }
  return sqlite3StrICmp(z1, z2)==0;
}


/*
** Check to see if index pSrc is compatible as a source of data
** for index pDest in an insert transfer optimization.  The rules
** for a compatible index:
**
**    *   The index is over the same set of columns
**    *   The same DESC and ASC markings occurs on all columns
**    *   The same onError processing (OE_Abort, OE_Ignore, etc)
**    *   The same collating sequence on each column
*/
static int xferCompatibleIndex(Index *pDest, Index *pSrc){
  int i;
  assert( pDest && pSrc );
  assert( pDest->pTable!=pSrc->pTable );
  if( pDest->nColumn!=pSrc->nColumn ){
    return 0;   /* Different number of columns */
  }
  if( pDest->onError!=pSrc->onError ){
    return 0;   /* Different conflict resolution strategies */
  }
  for(i=0; i<pSrc->nColumn; i++){
    if( pSrc->aiColumn[i]!=pDest->aiColumn[i] ){
      return 0;   /* Different columns indexed */
    }
    if( pSrc->aSortOrder[i]!=pDest->aSortOrder[i] ){
      return 0;   /* Different sort orders */
    }
    if( pSrc->azColl[i]!=pDest->azColl[i] ){
      return 0;   /* Different sort orders */
    }
  }

  /* If no test above fails then the indices must be compatible */
  return 1;
}

/*
** Attempt the transfer optimization on INSERTs of the form
**
**     INSERT INTO tab1 SELECT * FROM tab2;
**
** This optimization is only attempted if
**
**    (1)  tab1 and tab2 have identical schemas including all the
**         same indices and constraints
**
**    (2)  tab1 and tab2 are different tables
**
**    (3)  There must be no triggers on tab1
**
**    (4)  The result set of the SELECT statement is "*"
**
**    (5)  The SELECT statement has no WHERE, HAVING, ORDER BY, GROUP BY,
**         or LIMIT clause.
**
**    (6)  The SELECT statement is a simple (not a compound) select that
**         contains only tab2 in its FROM clause
**
** This method for implementing the INSERT transfers raw records from
** tab2 over to tab1.  The columns are not decoded.  Raw records from
** the indices of tab2 are transfered to tab1 as well.  In so doing,
** the resulting tab1 has much less fragmentation.
**
** This routine returns TRUE if the optimization is attempted.  If any
** of the conditions above fail so that the optimization should not
** be attempted, then this routine returns FALSE.
*/
static int xferOptimization(
  Parse *pParse,        /* Parser context */
  Table *pDest,         /* The table we are inserting into */
  Select *pSelect,      /* A SELECT statement to use as the data source */
  int onError,          /* How to handle constraint errors */
  int iDbDest           /* The database of pDest */
){
  ExprList *pEList;                /* The result set of the SELECT */
  Table *pSrc;                     /* The table in the FROM clause of SELECT */
  Index *pSrcIdx, *pDestIdx;       /* Source and destination indices */
  struct SrcList_item *pItem;      /* An element of pSelect->pSrc */
  int i;                           /* Loop counter */
  int iDbSrc;                      /* The database of pSrc */
  int iSrc, iDest;                 /* Cursors from source and destination */
  int addr1, addr2;                /* Loop addresses */
  int emptyDestTest;               /* Address of test for empty pDest */
  int emptySrcTest;                /* Address of test for empty pSrc */
  Vdbe *v;                         /* The VDBE we are building */
  KeyInfo *pKey;                   /* Key information for an index */
  int counterMem;                  /* Memory register used by AUTOINC */
  int destHasUniqueIdx = 0;        /* True if pDest has a UNIQUE index */

  if( pSelect==0 ){
    return 0;   /* Must be of the form  INSERT INTO ... SELECT ... */
  }
  if( pDest->pTrigger ){
    return 0;   /* tab1 must not have triggers */
  }
#ifndef SQLITE_OMIT_VIRTUALTABLE
  if( pDest->isVirtual ){
    return 0;   /* tab1 must not be a virtual table */
  }
#endif
  if( onError==OE_Default ){
    onError = OE_Abort;
  }
  if( onError!=OE_Abort && onError!=OE_Rollback ){
    return 0;   /* Cannot do OR REPLACE or OR IGNORE or OR FAIL */
  }
  if( pSelect->pSrc==0 ){
    return 0;   /* SELECT must have a FROM clause */
  }
  if( pSelect->pSrc->nSrc!=1 ){
    return 0;   /* FROM clause must have exactly one term */
  }
  if( pSelect->pSrc->a[0].pSelect ){
    return 0;   /* FROM clause cannot contain a subquery */
  }
  if( pSelect->pWhere ){
    return 0;   /* SELECT may not have a WHERE clause */
  }
  if( pSelect->pOrderBy ){
    return 0;   /* SELECT may not have an ORDER BY clause */
  }
  /* Do not need to test for a HAVING clause.  If HAVING is present but
  ** there is no ORDER BY, we will get an error. */
  if( pSelect->pGroupBy ){
    return 0;   /* SELECT may not have a GROUP BY clause */
  }
  if( pSelect->pLimit ){
    return 0;   /* SELECT may not have a LIMIT clause */
  }
  assert( pSelect->pOffset==0 );  /* Must be so if pLimit==0 */
  if( pSelect->pPrior ){
    return 0;   /* SELECT may not be a compound query */
  }
  if( pSelect->isDistinct ){
    return 0;   /* SELECT may not be DISTINCT */
  }
  pEList = pSelect->pEList;
  assert( pEList!=0 );
  if( pEList->nExpr!=1 ){
    return 0;   /* The result set must have exactly one column */
  }
  assert( pEList->a[0].pExpr );
  if( pEList->a[0].pExpr->op!=TK_ALL ){
    return 0;   /* The result set must be the special operator "*" */
  }

  /* At this point we have established that the statement is of the
  ** correct syntactic form to participate in this optimization.  Now
  ** we have to check the semantics.
  */
  pItem = pSelect->pSrc->a;
  pSrc = sqlite3LocateTable(pParse, pItem->zName, pItem->zDatabase);
  if( pSrc==0 ){
    return 0;   /* FROM clause does not contain a real table */
  }
  if( pSrc==pDest ){
    return 0;   /* tab1 and tab2 may not be the same table */
  }
#ifndef SQLITE_OMIT_VIRTUALTABLE
  if( pSrc->isVirtual ){
    return 0;   /* tab2 must not be a virtual table */
  }
#endif
  if( pSrc->pSelect ){
    return 0;   /* tab2 may not be a view */
  }
  if( pDest->nCol!=pSrc->nCol ){
    return 0;   /* Number of columns must be the same in tab1 and tab2 */
  }
  if( pDest->iPKey!=pSrc->iPKey ){
    return 0;   /* Both tables must have the same INTEGER PRIMARY KEY */
  }
  for(i=0; i<pDest->nCol; i++){
    if( pDest->aCol[i].affinity!=pSrc->aCol[i].affinity ){
      return 0;    /* Affinity must be the same on all columns */
    }
    if( !xferCompatibleCollation(pDest->aCol[i].zColl, pSrc->aCol[i].zColl) ){
      return 0;    /* Collating sequence must be the same on all columns */
    }
    if( pDest->aCol[i].notNull && !pSrc->aCol[i].notNull ){
      return 0;    /* tab2 must be NOT NULL if tab1 is */
    }
  }
  for(pDestIdx=pDest->pIndex; pDestIdx; pDestIdx=pDestIdx->pNext){
    if( pDestIdx->onError!=OE_None ){
      destHasUniqueIdx = 1;
    }
    for(pSrcIdx=pSrc->pIndex; pSrcIdx; pSrcIdx=pSrcIdx->pNext){
      if( xferCompatibleIndex(pDestIdx, pSrcIdx) ) break;
    }
    if( pSrcIdx==0 ){
      return 0;    /* pDestIdx has no corresponding index in pSrc */
    }
  }
#ifndef SQLITE_OMIT_CHECK
  if( pDest->pCheck && !sqlite3ExprCompare(pSrc->pCheck, pDest->pCheck) ){
    return 0;   /* Tables have different CHECK constraints.  Ticket #2252 */
  }
#endif

  /* If we get this far, it means either:
  **
  **    *   We can always do the transfer if the table contains an
  **        an integer primary key
  **
  **    *   We can conditionally do the transfer if the destination
  **        table is empty.
  */
#ifdef SQLITE_TEST
  sqlite3_xferopt_count++;
#endif
  iDbSrc = sqlite3SchemaToIndex(pParse->db, pSrc->pSchema);
  v = sqlite3GetVdbe(pParse);
  iSrc = pParse->nTab++;
  iDest = pParse->nTab++;
  counterMem = autoIncBegin(pParse, iDbDest, pDest);
  sqlite3OpenTable(pParse, iDest, iDbDest, pDest, OP_OpenWrite);
  if( (pDest->iPKey<0 && pDest->pIndex!=0) || destHasUniqueIdx ){
    /* If tables do not have an INTEGER PRIMARY KEY and there
    ** are indices to be copied and the destination is not empty,
    ** we have to disallow the transfer optimization because the
    ** the rowids might change which will mess up indexing.
    **
    ** Or if the destination has a UNIQUE index and is not empty,
    ** we also disallow the transfer optimization because we cannot
    ** insure that all entries in the union of DEST and SRC will be
    ** unique.
    */
    addr1 = sqlite3VdbeAddOp(v, OP_Rewind, iDest, 0);
    emptyDestTest = sqlite3VdbeAddOp(v, OP_Goto, 0, 0);
    sqlite3VdbeJumpHere(v, addr1);
  }else{
    emptyDestTest = 0;
  }
  sqlite3OpenTable(pParse, iSrc, iDbSrc, pSrc, OP_OpenRead);
  emptySrcTest = sqlite3VdbeAddOp(v, OP_Rewind, iSrc, 0);
  if( pDest->iPKey>=0 ){
    addr1 = sqlite3VdbeAddOp(v, OP_Rowid, iSrc, 0);
    sqlite3VdbeAddOp(v, OP_Dup, 0, 0);
    addr2 = sqlite3VdbeAddOp(v, OP_NotExists, iDest, 0);
    sqlite3VdbeOp3(v, OP_Halt, SQLITE_CONSTRAINT, onError, 
                      "PRIMARY KEY must be unique", P3_STATIC);
    sqlite3VdbeJumpHere(v, addr2);
    autoIncStep(pParse, counterMem);
  }else if( pDest->pIndex==0 ){
    addr1 = sqlite3VdbeAddOp(v, OP_NewRowid, iDest, 0);
  }else{
    addr1 = sqlite3VdbeAddOp(v, OP_Rowid, iSrc, 0);
    assert( pDest->autoInc==0 );
  }
  sqlite3VdbeAddOp(v, OP_RowData, iSrc, 0);
  sqlite3VdbeOp3(v, OP_Insert, iDest,
                    OPFLAG_NCHANGE|OPFLAG_LASTROWID|OPFLAG_APPEND,
                    pDest->zName, 0);
  sqlite3VdbeAddOp(v, OP_Next, iSrc, addr1);
  autoIncEnd(pParse, iDbDest, pDest, counterMem);
  for(pDestIdx=pDest->pIndex; pDestIdx; pDestIdx=pDestIdx->pNext){
    for(pSrcIdx=pSrc->pIndex; pSrcIdx; pSrcIdx=pSrcIdx->pNext){
      if( xferCompatibleIndex(pDestIdx, pSrcIdx) ) break;
    }
    assert( pSrcIdx );
    sqlite3VdbeAddOp(v, OP_Close, iSrc, 0);
    sqlite3VdbeAddOp(v, OP_Close, iDest, 0);
    sqlite3VdbeAddOp(v, OP_Integer, iDbSrc, 0);
    pKey = sqlite3IndexKeyinfo(pParse, pSrcIdx);
    VdbeComment((v, "# %s", pSrcIdx->zName));
    sqlite3VdbeOp3(v, OP_OpenRead, iSrc, pSrcIdx->tnum, 
                   (char*)pKey, P3_KEYINFO_HANDOFF);
    sqlite3VdbeAddOp(v, OP_Integer, iDbDest, 0);
    pKey = sqlite3IndexKeyinfo(pParse, pDestIdx);
    VdbeComment((v, "# %s", pDestIdx->zName));
    sqlite3VdbeOp3(v, OP_OpenWrite, iDest, pDestIdx->tnum, 
                   (char*)pKey, P3_KEYINFO_HANDOFF);
    addr1 = sqlite3VdbeAddOp(v, OP_Rewind, iSrc, 0);
    sqlite3VdbeAddOp(v, OP_RowKey, iSrc, 0);
    sqlite3VdbeAddOp(v, OP_IdxInsert, iDest, 1);
    sqlite3VdbeAddOp(v, OP_Next, iSrc, addr1+1);
    sqlite3VdbeJumpHere(v, addr1);
  }
  sqlite3VdbeJumpHere(v, emptySrcTest);
  sqlite3VdbeAddOp(v, OP_Close, iSrc, 0);
  sqlite3VdbeAddOp(v, OP_Close, iDest, 0);
  if( emptyDestTest ){
    sqlite3VdbeAddOp(v, OP_Halt, SQLITE_OK, 0);
    sqlite3VdbeJumpHere(v, emptyDestTest);
    sqlite3VdbeAddOp(v, OP_Close, iDest, 0);
    return 0;
  }else{
    return 1;
  }
}
#endif /* SQLITE_OMIT_XFER_OPT */
