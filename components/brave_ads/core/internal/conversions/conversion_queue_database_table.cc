/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/conversion_queue_database_table.h"

#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/containers/container_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_bind_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "conversion_queue";

constexpr int kDefaultBatchSize = 50;

int BindParameters(mojom::DBCommandInfo* command,
                   const ConversionQueueItemList& conversion_queue_items) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& conversion_queue_item : conversion_queue_items) {
    BindString(command, index++, conversion_queue_item.ad_type.ToString());
    BindString(command, index++, conversion_queue_item.campaign_id);
    BindString(command, index++, conversion_queue_item.creative_set_id);
    BindString(command, index++, conversion_queue_item.creative_instance_id);
    BindString(command, index++, conversion_queue_item.advertiser_id);
    BindString(command, index++, conversion_queue_item.segment);
    BindString(command, index++, conversion_queue_item.conversion_id);
    BindString(command, index++, conversion_queue_item.advertiser_public_key);
    BindDouble(command, index++, conversion_queue_item.process_at.ToDoubleT());
    BindInt(command, index++, int{conversion_queue_item.was_processed});

    count++;
  }

  return count;
}

ConversionQueueItemInfo GetFromRecord(mojom::DBRecordInfo* record) {
  DCHECK(record);

  ConversionQueueItemInfo conversion_queue_item;

  conversion_queue_item.ad_type = AdType(ColumnString(record, 0));
  conversion_queue_item.campaign_id = ColumnString(record, 1);
  conversion_queue_item.creative_set_id = ColumnString(record, 2);
  conversion_queue_item.creative_instance_id = ColumnString(record, 3);
  conversion_queue_item.advertiser_id = ColumnString(record, 4);
  conversion_queue_item.segment = ColumnString(record, 5);
  conversion_queue_item.conversion_id = ColumnString(record, 6);
  conversion_queue_item.advertiser_public_key = ColumnString(record, 7);
  conversion_queue_item.process_at =
      base::Time::FromDoubleT(ColumnDouble(record, 8));
  conversion_queue_item.was_processed = static_cast<bool>(ColumnInt(record, 9));

  return conversion_queue_item;
}

void OnGetAll(GetConversionQueueCallback callback,
              mojom::DBCommandResponseInfoPtr command_response) {
  if (!command_response ||
      command_response->status !=
          mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get conversion queue");
    return std::move(callback).Run(/*success*/ false,
                                   /*conversions_queue_items*/ {});
  }

  ConversionQueueItemList conversion_queue_items;

  for (const auto& record : command_response->result->get_records()) {
    const ConversionQueueItemInfo conversion_queue_item =
        GetFromRecord(record.get());
    conversion_queue_items.push_back(conversion_queue_item);
  }

  std::move(callback).Run(/*success*/ true, conversion_queue_items);
}

void OnGetForCreativeInstanceId(
    const std::string& creative_instance_id,
    GetConversionQueueForCreativeInstanceIdCallback callback,
    mojom::DBCommandResponseInfoPtr command_response) {
  if (!command_response ||
      command_response->status !=
          mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get conversion queue");
    return std::move(callback).Run(/*success*/ false, creative_instance_id,
                                   /*conversion_queue_items*/ {});
  }

  ConversionQueueItemList conversion_queue_items;

  for (const auto& record : command_response->result->get_records()) {
    const ConversionQueueItemInfo conversion_queue_item =
        GetFromRecord(record.get());
    conversion_queue_items.push_back(conversion_queue_item);
  }

  std::move(callback).Run(/*success*/ true, creative_instance_id,
                          conversion_queue_items);
}

void MigrateToV10(mojom::DBTransactionInfo* transaction) {
  DCHECK(transaction);

  DropTable(transaction, "conversion_queue");

  // campaign_id and advertiser_id can be NULL for legacy conversions migrated
  // from |ad_conversions.json| and conversion_id and advertiser_public_key will
  // be empty for non verifiable conversions
  const std::string query =
      "CREATE TABLE conversion_queue "
      "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
      "campaign_id TEXT, "
      "creative_set_id TEXT NOT NULL, "
      "creative_instance_id TEXT NOT NULL, "
      "advertiser_id TEXT, "
      "conversion_id TEXT, "
      "timestamp TIMESTAMP NOT NULL)";

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void MigrateToV11(mojom::DBTransactionInfo* transaction) {
  DCHECK(transaction);

  // Create a temporary table with new |advertiser_public_key| column
  const std::string query =
      "CREATE TABLE conversion_queue_temp "
      "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
      "campaign_id TEXT, "
      "creative_set_id TEXT NOT NULL, "
      "creative_instance_id TEXT NOT NULL, "
      "advertiser_id TEXT, "
      "conversion_id TEXT, "
      "advertiser_public_key TEXT, "
      "timestamp TIMESTAMP NOT NULL)";

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  // Copy columns to temporary table
  const std::vector<std::string> columns = {
      "campaign_id",   "creative_set_id", "creative_instance_id",
      "advertiser_id", "conversion_id",   "timestamp"};

  CopyTableColumns(transaction, "conversion_queue", "conversion_queue_temp",
                   columns, /*should_drop*/ true);

  // Rename temporary table
  RenameTable(transaction, "conversion_queue_temp", "conversion_queue");
}

void MigrateToV17(mojom::DBTransactionInfo* transaction) {
  DCHECK(transaction);

  CreateTableIndex(transaction, "conversion_queue", "creative_instance_id");
}

void MigrateToV21(mojom::DBTransactionInfo* transaction) {
  DCHECK(transaction);

  // Create a temporary table with new |ad_type| and |was_processed| column
  const std::string query =
      "CREATE TABLE conversion_queue_temp "
      "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
      "ad_type TEXT, "
      "campaign_id TEXT, "
      "creative_set_id TEXT NOT NULL, "
      "creative_instance_id TEXT NOT NULL, "
      "advertiser_id TEXT, "
      "conversion_id TEXT, "
      "advertiser_public_key TEXT, "
      "timestamp TIMESTAMP NOT NULL, "
      "was_processed INTEGER DEFAULT 0)";

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  // Copy columns to temporary table
  const std::vector<std::string> columns = {
      "campaign_id",   "creative_set_id", "creative_instance_id",
      "advertiser_id", "conversion_id",   "advertiser_public_key",
      "timestamp"};

  CopyTableColumns(transaction, "conversion_queue", "conversion_queue_temp",
                   columns, /*should_drop*/ true);

  // Rename temporary table
  RenameTable(transaction, "conversion_queue_temp", "conversion_queue");

  // Migrate legacy conversions
  const std::string update_query =
      "UPDATE conversion_queue "
      "SET ad_type = 'ad_notification' "
      "WHERE ad_type IS NULL";

  mojom::DBCommandInfoPtr update_command = mojom::DBCommandInfo::New();
  update_command->type = mojom::DBCommandInfo::Type::EXECUTE;
  update_command->command = update_query;

  transaction->commands.push_back(std::move(update_command));
}

void MigrateToV26(mojom::DBTransactionInfo* transaction) {
  DCHECK(transaction);

  // Create a temporary table with new |segment| column
  const std::string query =
      "CREATE TABLE conversion_queue_temp "
      "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
      "ad_type TEXT, "
      "campaign_id TEXT, "
      "creative_set_id TEXT NOT NULL, "
      "creative_instance_id TEXT NOT NULL, "
      "advertiser_id TEXT, "
      "segment TEXT, "
      "conversion_id TEXT, "
      "advertiser_public_key TEXT, "
      "timestamp TIMESTAMP NOT NULL, "
      "was_processed INTEGER DEFAULT 0)";

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  // Copy columns to temporary table
  const std::vector<std::string> columns = {"ad_type",
                                            "campaign_id",
                                            "creative_set_id",
                                            "creative_instance_id",
                                            "advertiser_id",
                                            "conversion_id",
                                            "advertiser_public_key",
                                            "timestamp",
                                            "was_processed"};

  CopyTableColumns(transaction, "conversion_queue", "conversion_queue_temp",
                   columns,
                   /*should_drop*/ true);

  // Rename temporary table
  RenameTable(transaction, "conversion_queue_temp", "conversion_queue");
}

}  // namespace

ConversionQueue::ConversionQueue() : batch_size_(kDefaultBatchSize) {}

void ConversionQueue::Save(
    const ConversionQueueItemList& conversion_queue_items,
    ResultCallback callback) {
  if (conversion_queue_items.empty()) {
    return std::move(callback).Run(/*success*/ true);
  }

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  const std::vector<ConversionQueueItemList> batches =
      SplitVector(conversion_queue_items, batch_size_);

  for (const auto& batch : batches) {
    InsertOrUpdate(transaction.get(), batch);
  }

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void ConversionQueue::Delete(
    const ConversionQueueItemInfo& conversion_queue_item,
    ResultCallback callback) const {
  const std::string query = base::ReplaceStringPlaceholders(
      "DELETE FROM $1 WHERE creative_instance_id = '$2'",
      {GetTableName(), conversion_queue_item.creative_instance_id}, nullptr);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->command = query;

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void ConversionQueue::Update(
    const ConversionQueueItemInfo& conversion_queue_item,
    ResultCallback callback) const {
  const std::string query = base::ReplaceStringPlaceholders(
      "UPDATE $1 SET was_processed = 1 WHERE was_processed == 0 AND "
      "creative_instance_id == '$2'",
      {GetTableName(), conversion_queue_item.creative_instance_id}, nullptr);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->command = query;

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void ConversionQueue::GetAll(GetConversionQueueCallback callback) const {
  const std::string query = base::ReplaceStringPlaceholders(
      "SELECT cq.ad_type, cq.campaign_id, cq.creative_set_id, "
      "cq.creative_instance_id, cq.advertiser_id, cq.segment, "
      "cq.conversion_id, cq.advertiser_public_key, cq.timestamp, "
      "cq.was_processed FROM $1 AS cq ORDER BY timestamp ASC",
      {GetTableName()}, nullptr);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->command = query;

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // ad_type
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // campaign_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // creative_instance_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // advertiser_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // segment
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // conversion_id
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // advertiser_public_key
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // process_at
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE      // was_processed
  };

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction), base::BindOnce(&OnGetAll, std::move(callback)));
}

void ConversionQueue::GetUnprocessed(
    GetConversionQueueCallback callback) const {
  const std::string query = base::ReplaceStringPlaceholders(
      "SELECT cq.ad_type, cq.campaign_id, cq.creative_set_id, "
      "cq.creative_instance_id, cq.advertiser_id, cq.segment, "
      "cq.conversion_id, cq.advertiser_public_key, cq.timestamp, "
      "cq.was_processed FROM $1 AS cq WHERE was_processed == 0 ORDER BY "
      "timestamp ASC",
      {GetTableName()}, nullptr);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->command = query;

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // ad_type
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // campaign_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // creative_instance_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // advertiser_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // segment
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // conversion_id
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // advertiser_public_key
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // process_at
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE      // was_processed
  };

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction), base::BindOnce(&OnGetAll, std::move(callback)));
}

void ConversionQueue::GetForCreativeInstanceId(
    const std::string& creative_instance_id,
    GetConversionQueueForCreativeInstanceIdCallback callback) const {
  if (creative_instance_id.empty()) {
    return std::move(callback).Run(/*success*/ false, creative_instance_id,
                                   /*conversion_queue_items*/ {});
  }

  const std::string query = base::ReplaceStringPlaceholders(
      "SELECT cq.ad_type, cq.campaign_id, cq.creative_set_id, "
      "cq.creative_instance_id, cq.advertiser_id, cq.segment, "
      "cq.conversion_id, cq.advertiser_public_key, cq.timestamp, "
      "cq.was_processed FROM $1 AS cq WHERE cq.creative_instance_id = '$2' "
      "ORDER BY timestamp ASC",
      {GetTableName(), creative_instance_id}, nullptr);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->command = query;

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // ad_type
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // campaign_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // creative_instance_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // advertiser_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // segment
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // conversion_id
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // advertiser_public_key
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // process_at
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE      // was_processed
  };

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnGetForCreativeInstanceId, creative_instance_id,
                     std::move(callback)));
}

std::string ConversionQueue::GetTableName() const {
  return kTableName;
}

void ConversionQueue::Migrate(mojom::DBTransactionInfo* transaction,
                              const int to_version) {
  DCHECK(transaction);

  switch (to_version) {
    case 10: {
      MigrateToV10(transaction);
      break;
    }

    case 11: {
      MigrateToV11(transaction);
      break;
    }

    case 17: {
      MigrateToV17(transaction);
      break;
    }

    case 21: {
      MigrateToV21(transaction);
      break;
    }

    case 26: {
      MigrateToV26(transaction);
      break;
    }

    default: {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void ConversionQueue::InsertOrUpdate(
    mojom::DBTransactionInfo* transaction,
    const ConversionQueueItemList& conversion_queue_items) {
  DCHECK(transaction);

  if (conversion_queue_items.empty()) {
    return;
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->command =
      BuildInsertOrUpdateQuery(command.get(), conversion_queue_items);

  transaction->commands.push_back(std::move(command));
}

std::string ConversionQueue::BuildInsertOrUpdateQuery(
    mojom::DBCommandInfo* command,
    const ConversionQueueItemList& conversion_queue_items) const {
  DCHECK(command);

  const int binded_parameters_count =
      BindParameters(command, conversion_queue_items);

  return base::ReplaceStringPlaceholders(
      "INSERT OR REPLACE INTO $1 (ad_type, campaign_id, creative_set_id, "
      "creative_instance_id, advertiser_id, segment, conversion_id, "
      "advertiser_public_key, timestamp, was_processed) VALUES $2",
      {GetTableName(), BuildBindingParameterPlaceholders(
                           /*parameters_count*/ 10, binded_parameters_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
