/* Copyright (c) 2010, 2013, Oracle and/or its affiliates. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation,
  51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA */

/**
  @file storage/perfschema/table_events_transactions.cc
  Table EVENTS_TRANSACTIONS_xxx (implementation).
*/

#include "my_global.h"
#include "my_pthread.h"
#include "table_events_transactions.h"
#include "pfs_instr_class.h"
#include "pfs_instr.h"
#include "pfs_events_transactions.h"
#include "pfs_timer.h"
#include "table_helper.h"

THR_LOCK table_events_transactions_current::m_table_lock;

static const TABLE_FIELD_TYPE field_types[]=
{
  {
    { C_STRING_WITH_LEN("THREAD_ID") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("EVENT_ID") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("END_EVENT_ID") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("EVENT_NAME") },
    { C_STRING_WITH_LEN("varchar(128)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("STATE") },
    { C_STRING_WITH_LEN("enum(\'ACTIVE\',\'COMMITTED\',\'ROLLED BACK\'") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("TRX_ID") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("GTID") },
    { C_STRING_WITH_LEN("varchar(64)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("XID") },
    { C_STRING_WITH_LEN("varchar(132") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("XA_STATE") },
    { C_STRING_WITH_LEN("varchar(64)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SOURCE") },
    { C_STRING_WITH_LEN("varchar(64)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("TIMER_START") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("TIMER_END") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("TIMER_WAIT") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("ACCESS_MODE") },
    { C_STRING_WITH_LEN("enum(\'READ ONLY\',\'READ WRITE\'") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("ISOLATION_LEVEL") },
    { C_STRING_WITH_LEN("varchar(64)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("AUTOCOMMIT") },
    { C_STRING_WITH_LEN("enum(\'YES\',\'NO\'") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("NUMBER_OF_SAVEPOINTS") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("NUMBER_OF_ROLLBACK_TO_SAVEPOINT") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("NUMBER_OF_RELEASE_SAVEPOINT") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("OBJECT_INSTANCE_BEGIN") },
    { C_STRING_WITH_LEN("bigint") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("NESTING_EVENT_ID") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("NESTING_EVENT_TYPE") },
    { C_STRING_WITH_LEN("enum(\'TRANSACTION\',\'STATEMENT\',\'STAGE\',\'WAIT\'") },
    { NULL, 0}
  }
};

TABLE_FIELD_DEF
table_events_transactions_current::m_field_def=
{22 , field_types };

PFS_engine_table_share
table_events_transactions_current::m_share=
{
  { C_STRING_WITH_LEN("events_transactions_current") },
  &pfs_truncatable_acl,
  &table_events_transactions_current::create,
  NULL, /* write_row */
  &table_events_transactions_current::delete_all_rows,
  NULL, /* get_row_count */
  1000, /* records */
  sizeof(PFS_simple_index), /* ref length */
  &m_table_lock,
  &m_field_def,
  false /* checked */
};

THR_LOCK table_events_transactions_history::m_table_lock;

PFS_engine_table_share
table_events_transactions_history::m_share=
{
  { C_STRING_WITH_LEN("events_transactions_history") },
  &pfs_truncatable_acl,
  &table_events_transactions_history::create,
  NULL, /* write_row */
  &table_events_transactions_history::delete_all_rows,
  NULL, /* get_row_count */
  1000, /* records */
  sizeof(pos_events_transactions_history), /* ref length */
  &m_table_lock,
  &table_events_transactions_current::m_field_def,
  false /* checked */
};

THR_LOCK table_events_transactions_history_long::m_table_lock;

PFS_engine_table_share
table_events_transactions_history_long::m_share=
{
  { C_STRING_WITH_LEN("events_transactions_history_long") },
  &pfs_truncatable_acl,
  &table_events_transactions_history_long::create,
  NULL, /* write_row */
  &table_events_transactions_history_long::delete_all_rows,
  NULL, /* get_row_count */
  10000, /* records */
  sizeof(PFS_simple_index), /* ref length */
  &m_table_lock,
  &table_events_transactions_current::m_field_def,
  false /* checked */
};

table_events_transactions_common::table_events_transactions_common
(const PFS_engine_table_share *share, void *pos)
  : PFS_engine_table(share, pos),
  m_row_exists(false)
{}

/**
  Build a row.
  @param transaction                      the transaction the cursor is reading
*/
void table_events_transactions_common::make_row(PFS_events_transactions *transaction)
{
  const char *base;
  const char *safe_source_file;

  m_row_exists= false;

  PFS_transaction_class *unsafe= (PFS_transaction_class*) transaction->m_class;
  PFS_transaction_class *klass= sanitize_transaction_class(unsafe);
  if (unlikely(klass == NULL))
    return;

  m_row.m_thread_internal_id= transaction->m_thread_internal_id;
  m_row.m_event_id= transaction->m_event_id;
  m_row.m_end_event_id= transaction->m_end_event_id;
  m_row.m_nesting_event_id= transaction->m_nesting_event_id;
  m_row.m_nesting_event_type= transaction->m_nesting_event_type;

  m_normalizer->to_pico(transaction->m_timer_start, transaction->m_timer_end,
                        &m_row.m_timer_start, &m_row.m_timer_end, &m_row.m_timer_wait);
  m_row.m_name= klass->m_name;
  m_row.m_name_length= klass->m_name_length;

  safe_source_file= transaction->m_source_file;
  if (unlikely(safe_source_file == NULL))
    return;

  base= base_name(safe_source_file);
  m_row.m_source_length= (uint)my_snprintf(m_row.m_source, sizeof(m_row.m_source),
                                     "%s:%d", base, transaction->m_source_line);
  if (m_row.m_source_length > sizeof(m_row.m_source))
    m_row.m_source_length= sizeof(m_row.m_source);

  if (transaction->m_gtid_set)
  {
    /* A GTID consists of the SIDNO (source id) and GNO (transaction number).
       The SIDNO is used internally. It maps to a UUID, here called SID.
    */
    rpl_sid *sid= &transaction->m_sid;

    /* The Gtid_specification contains the SIDNO, GNO and TYPE. Given a SID,
       it can generate the textual representation of the GTID. Without
       the SID, the Gtid_spec would first have to map the SIDNO to the SID,
       which requires locking the global SID map. Fortunately, mapping
       from SIDNO to SID was already done at the time of logging.
    */
    Gtid_specification *gtid_spec= &transaction->m_gtid_spec;
    m_row.m_gtid_length= gtid_spec->to_string(sid, m_row.m_gtid);
  }
  else
    m_row.m_gtid_length= 0;
  
  m_row.m_xid= transaction->m_xid;
  m_row.m_isolation_level= transaction->m_isolation_level;
  m_row.m_read_only= transaction->m_read_only;
  m_row.m_trxid= transaction->m_trxid;
  m_row.m_state= transaction->m_state;
  m_row.m_xa_state= transaction->m_xa_state;
  m_row.m_xa= transaction->m_xa;
  m_row.m_autocommit= transaction->m_autocommit;
  m_row.m_savepoint_count= transaction->m_savepoint_count;
  m_row.m_rollback_to_savepoint_count= transaction->m_rollback_to_savepoint_count;
  m_row.m_release_savepoint_count= transaction->m_release_savepoint_count;
  m_row_exists= true;
  return;
}

/**
  Convert the XID to HEX string prefixed by '0x' 
  @buf has to be at least (XIDDATASIZE*2+2+1) in size
*/
static uint xid_to_hex(char *buf, size_t buf_len, PSI_xid *xid)
{
  *buf++= '0';
  *buf++= 'x';
  return bin_to_hex_str(buf, buf_len-2, (char*)&xid->data,
                        xid->gtrid_length + xid->bqual_length) + 2;
}

/**
  Store the XID in printable format if possible, otherwise convert
  to a string of hex digits.
*/
static void xid_store(Field *field, PSI_xid *xid)
{
  if(xid_printable(xid))
  {
    field->store(xid->data, xid->gtrid_length + xid->bqual_length,
                 &my_charset_bin);
  }
  else
  {
    uint xid_str_len;
    /* 
      xid_buf contains enough space for 0x followed by hex representation of
      the binary XID data and one null termination character.
    */
    char xid_buf[XIDDATASIZE * 2 + 2 + 1];

    xid_str_len= xid_to_hex(xid_buf, sizeof(xid_buf), xid);
    field->store(xid_buf, xid_str_len, &my_charset_bin);
  }
}

int table_events_transactions_common::read_row_values(TABLE *table,
                                                      unsigned char *buf,
                                                      Field **fields,
                                                      bool read_all)
{
  Field *f;

  if (unlikely(! m_row_exists))
    return HA_ERR_RECORD_DELETED;

  /* Set the null bits */
  DBUG_ASSERT(table->s->null_bytes == 3);
  buf[0]= 0;
  buf[1]= 0;
  buf[2]= 0;

  for (; (f= *fields) ; fields++)
  {
    if (read_all || bitmap_is_set(table->read_set, f->field_index))
    {
      switch(f->field_index)
      {
      case 0: /* THREAD_ID */
        set_field_ulonglong(f, m_row.m_thread_internal_id);
        break;
      case 1: /* EVENT_ID */
        set_field_ulonglong(f, m_row.m_event_id);
        break;
      case 2: /* END_EVENT_ID */
        if (m_row.m_end_event_id > 0)
          set_field_ulonglong(f, m_row.m_end_event_id - 1);
        else
          f->set_null();
        break;
      case 3: /* EVENT_NAME */
        set_field_varchar_utf8(f, m_row.m_name, m_row.m_name_length);
        break;
      case 4: /* STATE */
        set_field_enum(f, m_row.m_state);
        break;
      case 5: /* TRX_ID */
        if (m_row.m_trxid != 0)
          set_field_ulonglong(f, m_row.m_trxid);
        else
          f->set_null();
        break;
      case 6: /* GTID */
        if (m_row.m_gtid_length == 0)
          f->set_null();
        else
          set_field_varchar_utf8(f, m_row.m_gtid, m_row.m_gtid_length);
        break;
      case 7: /* XID */
        if (!m_row.m_xa || m_row.m_xid.is_null())
          f->set_null();
        else
          xid_store(f, &m_row.m_xid);
        break;
      case 8: /* XA STATE */
        if (!m_row.m_xa || m_row.m_xid.is_null())
          f->set_null();
        else
          set_field_xa_state(f, m_row.m_xa_state);
        break;
      case 9: /* SOURCE */
        set_field_varchar_utf8(f, m_row.m_source, m_row.m_source_length);
        break;
      case 10: /* TIMER_START */
        if (m_row.m_timer_start != 0)
          set_field_ulonglong(f, m_row.m_timer_start);
        else
          f->set_null();
        break;
      case 11: /* TIMER_END */
        if (m_row.m_timer_end != 0)
          set_field_ulonglong(f, m_row.m_timer_end);
        else
          f->set_null();
        break;
      case 12: /* TIMER_WAIT */
        if (m_row.m_timer_wait != 0)
          set_field_ulonglong(f, m_row.m_timer_wait);
        else
          f->set_null();
        break;
      case 13: /* ACCESS_MODE */
        set_field_enum(f, m_row.m_read_only ? TRANS_MODE_READ_ONLY
                                            : TRANS_MODE_READ_WRITE);
        break;
      case 14: /* ISOLATION_LEVEL */
        set_field_isolation_level(f, m_row.m_isolation_level);
        break;
      case 15: /* AUTOCOMMIT */
        set_field_enum(f, m_row.m_autocommit ? ENUM_YES : ENUM_NO);
        break;
      case 16: /* NUMBER_OF_SAVEPOINTS */
        set_field_ulonglong(f, m_row.m_savepoint_count);
        break;
      case 17: /* NUMBER_OF_ROLLBACK_TO_SAVEPOINT */
        set_field_ulonglong(f, m_row.m_rollback_to_savepoint_count);
        break;
      case 18: /* NUMBER_OF_RELEASE_SAVEPOINT */
        set_field_ulonglong(f, m_row.m_release_savepoint_count);
        break;
      case 19: /* OBJECT_INSTANCE_BEGIN */
        f->set_null();
        break;
      case 20: /* NESTING_EVENT_ID */
        if (m_row.m_nesting_event_id != 0)
          set_field_ulonglong(f, m_row.m_nesting_event_id);
        else
          f->set_null();
        break;
      case 21: /* NESTING_EVENT_TYPE */
        if (m_row.m_nesting_event_id != 0)
          set_field_enum(f, m_row.m_nesting_event_type);
        else
          f->set_null();
        break;
      default:
        DBUG_ASSERT(false);
      }
    }
  }
  return 0;
}

PFS_engine_table* table_events_transactions_current::create(void)
{
  return new table_events_transactions_current();
}

table_events_transactions_current::table_events_transactions_current()
  : table_events_transactions_common(&m_share, &m_pos),
  m_pos(0), m_next_pos(0)
{}

void table_events_transactions_current::reset_position(void)
{
  m_pos.m_index= 0;
  m_next_pos.m_index= 0;
}

int table_events_transactions_current::rnd_init(bool scan)
{
  m_normalizer= time_normalizer::get(transaction_timer);
  return 0;
}

int table_events_transactions_current::rnd_next(void)
{
  PFS_thread *pfs_thread;
  PFS_events_transactions *transaction;

  for (m_pos.set_at(&m_next_pos);
       m_pos.m_index < thread_max;
       m_pos.next())
  {
    pfs_thread= &thread_array[m_pos.m_index];

    if (!pfs_thread->m_lock.is_populated())
    {
      /* This thread does not exist */
      continue;
    }

    transaction= &pfs_thread->m_transaction_current;

    make_row(transaction);
    m_next_pos.set_after(&m_pos);
    return 0;
  }

  return HA_ERR_END_OF_FILE;
}

int table_events_transactions_current::rnd_pos(const void *pos)
{
  PFS_thread *pfs_thread;
  PFS_events_transactions *transaction;

  set_position(pos);
  DBUG_ASSERT(m_pos.m_index < thread_max);
  pfs_thread= &thread_array[m_pos.m_index];

  if (!pfs_thread->m_lock.is_populated())
    return HA_ERR_RECORD_DELETED;

  transaction= &pfs_thread->m_transaction_current;

  if (transaction->m_class == NULL)
    return HA_ERR_RECORD_DELETED;

  make_row(transaction);
  return 0;
}

int table_events_transactions_current::delete_all_rows(void)
{
  reset_events_transactions_current();
  return 0;
}

PFS_engine_table* table_events_transactions_history::create(void)
{
  return new table_events_transactions_history();
}

table_events_transactions_history::table_events_transactions_history()
  : table_events_transactions_common(&m_share, &m_pos),
  m_pos(), m_next_pos()
{}

void table_events_transactions_history::reset_position(void)
{
  m_pos.reset();
  m_next_pos.reset();
}

int table_events_transactions_history::rnd_init(bool scan)
{
  m_normalizer= time_normalizer::get(transaction_timer);
  return 0;
}

int table_events_transactions_history::rnd_next(void)
{
  PFS_thread *pfs_thread;
  PFS_events_transactions *transaction;

  if (events_transactions_history_per_thread == 0)
    return HA_ERR_END_OF_FILE;

  for (m_pos.set_at(&m_next_pos);
       m_pos.m_index_1 < thread_max;
       m_pos.next_thread())
  {
    pfs_thread= &thread_array[m_pos.m_index_1];

    if (! pfs_thread->m_lock.is_populated())
    {
      /* This thread does not exist */
      continue;
    }

    if (m_pos.m_index_2 >= events_transactions_history_per_thread)
    {
      /* This thread does not have more (full) history */
      continue;
    }

    if ( ! pfs_thread->m_transactions_history_full &&
        (m_pos.m_index_2 >= pfs_thread->m_transactions_history_index))
    {
      /* This thread does not have more (not full) history */
      continue;
    }

    transaction= &pfs_thread->m_transactions_history[m_pos.m_index_2];

    if (transaction->m_class != NULL)
    {
      make_row(transaction);
      /* Next iteration, look for the next history in this thread */
      m_next_pos.set_after(&m_pos);
      return 0;
    }
  }

  return HA_ERR_END_OF_FILE;
}

int table_events_transactions_history::rnd_pos(const void *pos)
{
  PFS_thread *pfs_thread;
  PFS_events_transactions *transaction;

  DBUG_ASSERT(events_transactions_history_per_thread != 0);
  set_position(pos);
  DBUG_ASSERT(m_pos.m_index_1 < thread_max);
  pfs_thread= &thread_array[m_pos.m_index_1];

  if (! pfs_thread->m_lock.is_populated())
    return HA_ERR_RECORD_DELETED;

  DBUG_ASSERT(m_pos.m_index_2 < events_transactions_history_per_thread);

  if ( ! pfs_thread->m_transactions_history_full &&
      (m_pos.m_index_2 >= pfs_thread->m_transactions_history_index))
    return HA_ERR_RECORD_DELETED;

  transaction= &pfs_thread->m_transactions_history[m_pos.m_index_2];

  if (transaction->m_class == NULL)
    return HA_ERR_RECORD_DELETED;

  make_row(transaction);
  return 0;
}

int table_events_transactions_history::delete_all_rows(void)
{
  reset_events_transactions_history();
  return 0;
}

PFS_engine_table* table_events_transactions_history_long::create(void)
{
  return new table_events_transactions_history_long();
}

table_events_transactions_history_long::table_events_transactions_history_long()
  : table_events_transactions_common(&m_share, &m_pos),
  m_pos(0), m_next_pos(0)
{}

void table_events_transactions_history_long::reset_position(void)
{
  m_pos.m_index= 0;
  m_next_pos.m_index= 0;
}

int table_events_transactions_history_long::rnd_init(bool scan)
{
  m_normalizer= time_normalizer::get(transaction_timer);
  return 0;
}

int table_events_transactions_history_long::rnd_next(void)
{
  PFS_events_transactions *transaction;
  uint limit;

  if (events_transactions_history_long_size == 0)
    return HA_ERR_END_OF_FILE;

  if (events_transactions_history_long_full)
    limit= events_transactions_history_long_size;
  else
    limit= events_transactions_history_long_index.m_u32 % events_transactions_history_long_size;

  for (m_pos.set_at(&m_next_pos); m_pos.m_index < limit; m_pos.next())
  {
    transaction= &events_transactions_history_long_array[m_pos.m_index];

    if (transaction->m_class != NULL)
    {
      make_row(transaction);
      /* Next iteration, look for the next entry */
      m_next_pos.set_after(&m_pos);
      return 0;
    }
  }

  return HA_ERR_END_OF_FILE;
}

int table_events_transactions_history_long::rnd_pos(const void *pos)
{
  PFS_events_transactions *transaction;
  uint limit;

  if (events_transactions_history_long_size == 0)
    return HA_ERR_RECORD_DELETED;

  set_position(pos);

  if (events_transactions_history_long_full)
    limit= events_transactions_history_long_size;
  else
    limit= events_transactions_history_long_index.m_u32 % events_transactions_history_long_size;

  if (m_pos.m_index >= limit)
    return HA_ERR_RECORD_DELETED;

  transaction= &events_transactions_history_long_array[m_pos.m_index];

  if (transaction->m_class == NULL)
    return HA_ERR_RECORD_DELETED;

  make_row(transaction);
  return 0;
}

int table_events_transactions_history_long::delete_all_rows(void)
{
  reset_events_transactions_history_long();
  return 0;
}
