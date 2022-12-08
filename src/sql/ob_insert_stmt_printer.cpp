/**
 * Copyright (c) 2021 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#define USING_LOG_PREFIX SQL
#include "sql/ob_insert_stmt_printer.h"
#include "sql/ob_sql_context.h"
#include "sql/ob_select_stmt_printer.h"

namespace oceanbase
{
using namespace common;
namespace sql
{

void ObInsertStmtPrinter::init(char *buf, int64_t buf_len, int64_t *pos, ObInsertStmt *stmt)
{
  ObDMLStmtPrinter::init(buf, buf_len, pos, stmt);
}

int ObInsertStmtPrinter::do_print()
{
  int ret = OB_SUCCESS;

  if (OB_UNLIKELY(!is_inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not inited!", K(ret));
  } else if (OB_ISNULL(stmt_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("stmt should not be NULL", K(ret));
  } else {
    expr_printer_.init(buf_, buf_len_, pos_, print_params_);
    if (OB_FAIL(print())) {
      LOG_WARN("fail to print stmt", K(ret));
    }
  }

  return ret;
}


int ObInsertStmtPrinter::print()
{
  int ret = OB_SUCCESS;

  if (OB_ISNULL(stmt_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("stmt_ should not be NULL", K(ret));
  } else if (OB_UNLIKELY(!stmt_->is_insert_stmt())) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("Not a valid insert stmt", K(stmt_->get_stmt_type()),K(ret));
  } else {
    const ObInsertStmt *insert_stmt = static_cast<const ObInsertStmt*>(stmt_);
    if (OB_FAIL(print_basic_stmt())) {
      LOG_WARN("fail to print basic stmt", K(ret), K(*stmt_));
    } else { /*do nothing*/ }
  }

  return ret;
}

int ObInsertStmtPrinter::print_basic_stmt()
{
  int ret = OB_SUCCESS;

  if (OB_ISNULL(stmt_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("stmt_ should not be NULL", K(ret));
  } else if (OB_FAIL(print_insert())) {
    LOG_WARN("fail to print select", K(ret), K(*stmt_));
  } else if (OB_FAIL(print_into())) {
    LOG_WARN("fail to print into", K(ret), K(*stmt_));
  } else if (OB_FAIL(print_values())) {
    LOG_WARN("fail to print values", K(ret), K(*stmt_));
  } else if (OB_FAIL(print_returning())) {
    LOG_WARN("fail to print_returning", K(ret), K(*stmt_));
  } else {
    // do-nothing
  }

  return ret;
}

int ObInsertStmtPrinter::print_insert()
{
  int ret = OB_SUCCESS;

  if (OB_ISNULL(stmt_) || OB_ISNULL(buf_) || OB_ISNULL(pos_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("stmt_ is NULL or buf_ is NULL or pos_ is NULL", K(ret));
  } else {
    DATA_PRINTF("insert ");
    if (OB_SUCC(ret)) {
      if (OB_FAIL(print_hint())) { // hint
        LOG_WARN("fail to print hint", K(ret), K(*stmt_));
      } else {
        DATA_PRINTF("into ");
      }
    }
  }

  return ret;
}

int ObInsertStmtPrinter::print_into()
{
  int ret = OB_SUCCESS;
  if (OB_ISNULL(stmt_) || OB_ISNULL(buf_) || OB_ISNULL(pos_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("stmt_ is NULL or buf_ is NULL or pos_ is NULL", K(ret));
  } else {
    const ObInsertStmt *insert_stmt = static_cast<const ObInsertStmt*>(stmt_);
    const TableItem *table_item = NULL;
    if (OB_ISNULL(table_item = insert_stmt->get_table_item(0))) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("Invalid table item", K(stmt_->get_table_size()), K(ret));
    } else if (OB_FAIL(print_table(table_item))) {
      LOG_WARN("failed to print table", K(*table_item), K(ret));
    } else {
      DATA_PRINTF("(");
      for (int64_t i = 0; OB_SUCC(ret) && i < insert_stmt->get_values_desc().count(); ++i) {
        const ObColumnRefRawExpr* column = insert_stmt->get_values_desc().at(i);
        if (OB_ISNULL(column)) {
          ret = OB_ERR_UNEXPECTED;
          LOG_WARN("column is NULL", K(ret));
        } else if (column->get_column_id() == OB_HIDDEN_SESSION_ID_COLUMN_ID
                   || column->get_column_id() == OB_HIDDEN_SESS_CREATE_TIME_COLUMN_ID) {
          // 临时表的隐藏列，不需要print。TODO 将临时表insert改写由resolver转移到改写阶段后，可以去掉本分支
          LOG_DEBUG("do not print column", K(*column));
        } else {
          DATA_PRINTF(" \"");
          PRINT_IDENT(column->get_column_name());
          DATA_PRINTF("\",");
        }
      }
      if (OB_SUCC(ret)) {
        --*pos_;
        DATA_PRINTF(") ");
      }
    }
  }
  return ret;
}

int ObInsertStmtPrinter::print_values()
{
  int ret = OB_SUCCESS;
  if (OB_ISNULL(stmt_) || OB_ISNULL(buf_) || OB_ISNULL(pos_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("stmt_ is NULL or buf_ is NULL or pos_ is NULL", K(ret));
  } else if (!stmt_->is_insert_stmt()) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("Not a valid insert stmt", K(stmt_->get_stmt_type()), K(stmt_->get_table_size()), K(ret));
  } else {
    const ObInsertStmt *insert_stmt = static_cast<const ObInsertStmt*>(stmt_);
    if (insert_stmt->get_from_item_size() == 1) {
      const TableItem *view = insert_stmt->get_table_item_by_id(
                                insert_stmt->get_from_item(0).table_id_);
      const ObSelectStmt* sub_select_stmt = NULL;
      if (OB_ISNULL(view) || OB_ISNULL(sub_select_stmt = view->ref_query_)) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("sub select stmt is null", K(ret), K(sub_select_stmt));
      } else {
        ObSelectStmtPrinter printer(
              buf_, buf_len_, pos_,
              static_cast<const ObSelectStmt*>(sub_select_stmt), print_params_, NULL, false);
        if (OB_FAIL(printer.do_print())) {
          LOG_WARN("failed to print sub select printer", K(ret));
        }
      }
    } else {
      if (insert_stmt->get_values_desc().empty()) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("value desc is empty", K(insert_stmt->get_values_desc()), K(ret));
      } else {
        int64_t column_count = insert_stmt->get_values_desc().count();
        int64_t row_count = insert_stmt->get_values_vector().count() / column_count;
        DATA_PRINTF("values");
        for (int64_t i = 0; OB_SUCC(ret) && i < row_count; ++i) {
          DATA_PRINTF("(");
          for (int64_t j = 0; OB_SUCC(ret) && j < column_count; ++j) {
            const ObColumnRefRawExpr* column = insert_stmt->get_values_desc().at(j);
            if (OB_ISNULL(column)) {
              ret = OB_ERR_UNEXPECTED;
              LOG_WARN("column is NULL", K(ret));
            } else if (column->get_column_id() == OB_HIDDEN_SESSION_ID_COLUMN_ID
                       || column->get_column_id() == OB_HIDDEN_SESS_CREATE_TIME_COLUMN_ID) {
              // 临时表的隐藏列，不需要print。TODO 将临时表insert改写由resolver转移到改写阶段后，可以去掉本分支
              LOG_DEBUG("do not print column", K(*column));
            } else if (OB_FAIL(expr_printer_.do_print(insert_stmt->get_values_vector().at(i * column_count + j), T_INSERT_SCOPE))) {
              LOG_WARN("fail to print where expr", K(ret));
            } else {
              DATA_PRINTF(",");
            }
          }
          if (OB_SUCC(ret)) {
            --*pos_;
            DATA_PRINTF("),");
          }
        }
        if (OB_SUCC(ret)) {
          --*pos_;
        }
      }
    }
  }
  return ret;
}

int ObInsertStmtPrinter::print_subquery(const ObInsertStmt *insert_stmt)
{
  int ret = OB_SUCCESS;
  if (OB_ISNULL(insert_stmt)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("insert_stmt is NULL", K(ret), K(insert_stmt));
  } else {
    const TableItem *table_item = insert_stmt->get_table_item(insert_stmt->get_table_size() - 1);
    const ObSelectStmt* sub_select_stmt = NULL;
    if (OB_ISNULL(table_item) || OB_UNLIKELY(!table_item->is_generated_table()) ||
        OB_ISNULL(sub_select_stmt = table_item->ref_query_)) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("sub select stmt is null", K(ret), K(table_item), K(sub_select_stmt));
    } else {
      ObSelectStmtPrinter printer(buf_, buf_len_, pos_,
                                  static_cast<const ObSelectStmt*>(sub_select_stmt),
                                  print_params_, NULL, false, false, true);
      if (OB_FAIL(printer.do_print())) {
        LOG_WARN("failed to print sub select printer", K(ret));
      }
    }
  }
  return ret;
}

} //end of namespace sql
} //end of namespace oceanbase


