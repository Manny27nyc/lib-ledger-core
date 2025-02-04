/*
 *
 * EthereumLikeAccount
 *
 * Created by El Khalil Bellakrid on 12/07/2018.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Ledger
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */


#include "EthereumLikeAccount.h"
#include "EthereumLikeWallet.h"
#include <api/ERC20Token.hpp>
#include <api_impl/BigIntImpl.hpp>
#include <wallet/common/database/OperationDatabaseHelper.h>
#include <wallet/common/synchronizers/AbstractBlockchainExplorerAccountSynchronizer.h>
#include <wallet/ethereum/database/EthereumLikeAccountDatabaseHelper.h>
#include <wallet/ethereum/explorers/EthereumLikeBlockchainExplorer.h>
#include <wallet/ethereum/keychains/EthereumLikeKeychain.hpp>
#include <wallet/ethereum/transaction_builders/EthereumLikeTransactionBuilder.h>
#include <wallet/ethereum/database/EthereumLikeTransactionDatabaseHelper.h>
#include <wallet/ethereum/api_impl/EthereumLikeTransactionApi.h>
#include <wallet/ethereum/api_impl/InternalTransaction.h>
#include <wallet/ethereum/ERC20/erc20Tokens.h>
#include <wallet/ethereum/ERC20/ERC20LikeOperation.h>
#include <wallet/common/database/BlockDatabaseHelper.h>
#include <wallet/pool/database/CurrenciesDatabaseHelper.hpp>
#include <wallet/pool/WalletPool.hpp>
#include <events/Event.hpp>
#include <math/Base58.hpp>
#include <utils/Option.hpp>
#include <utils/DateUtils.hpp>
#include <wallet/ethereum/database/EthereumLikeOperationDatabaseHelper.hpp>

#include <database/soci-number.h>
#include <database/soci-date.h>
#include <database/soci-option.h>
#include <wallet/common/database/BulkInsertDatabaseHelper.hpp>


namespace ledger {
    namespace core {

        EthereumLikeAccount::EthereumLikeAccount(const std::shared_ptr<AbstractWallet>& wallet,
                                                 int32_t index,
                                                 const std::shared_ptr<EthereumLikeBlockchainExplorer>& explorer,
                                                 const std::shared_ptr<EthereumLikeAccountSynchronizer>& synchronizer,
                                                 const std::shared_ptr<EthereumLikeKeychain>& keychain): AbstractAccount(wallet, index) {
            _explorer = explorer;
            _synchronizer = synchronizer;
            _keychain = keychain;
            _accountAddress = keychain->getAddress()->toString();
        }


        FuturePtr<EthereumLikeBlockchainExplorerTransaction> EthereumLikeAccount::getTransaction(const std::string& hash) {
                auto self = std::dynamic_pointer_cast<EthereumLikeAccount>(shared_from_this());
                return async<std::shared_ptr<EthereumLikeBlockchainExplorerTransaction>>([=] () -> std::shared_ptr<EthereumLikeBlockchainExplorerTransaction> {
                    auto tx = std::make_shared<EthereumLikeBlockchainExplorerTransaction>();
                    soci::session sql(self->getWallet()->getDatabase()->getReadonlyPool());
                    if (!EthereumLikeTransactionDatabaseHelper::getTransactionByHash(sql, hash, *tx)) {
                            throw make_exception(api::ErrorCode::TRANSACTION_NOT_FOUND, "Transaction {} not found", hash);
                    }
                    return tx;
                });
        }

        void EthereumLikeAccount::inflateOperation(Operation &out,
                                                   const std::shared_ptr<const AbstractWallet>& wallet,
                                                   const EthereumLikeBlockchainExplorerTransaction &tx) {
                out.accountUid = getAccountUid();
                out.block = tx.block;
                out.ethereumTransaction = Option<EthereumLikeBlockchainExplorerTransaction>(tx);
                out.currencyName = getWallet()->getCurrency().name;
                out.walletType = getWalletType();
                out.walletUid = wallet->getWalletUid();
                out.date = tx.receivedAt;
                if (out.block.nonEmpty())
                        out.block.getValue().currencyName = wallet->getCurrency().name;
                out.ethereumTransaction.getValue().block = out.block;
        }

        void EthereumLikeAccount::interpretTransaction(
                const ledger::core::EthereumLikeBlockchainExplorerTransaction &transaction,
                std::vector<Operation> &out) {
            auto wallet = getWallet();
            if (wallet == nullptr) {
                throw Exception(api::ErrorCode::RUNTIME_ERROR, "Wallet reference is dead.");
            }

            int result = FLAG_TRANSACTION_IGNORED;

            Operation operation;
            inflateOperation(operation, wallet, transaction);
            std::vector<std::string> senders { transaction.sender };
            operation.senders = std::move(senders);
            std::vector<std::string> receivers { transaction.receiver };
            operation.recipients = std::move(receivers);
            operation.fees = transaction.gasPrice * transaction.gasUsed.getValueOr(BigInt::ZERO);
            operation.trust = std::make_shared<TrustIndicator>();
            operation.date = transaction.receivedAt;

            auto updateOperation = [&] (Operation &operation, api::OperationType ty) mutable {
                // if the status of the transaction is not correct, we set the operation’s amount to
                // zero as it’s failed (yet fees were still paid)
                if (transaction.status == 0  && transaction.block.nonEmpty()) {
                    operation.amount = BigInt::ZERO;
                } else {
                    operation.amount = transaction.value;
                }

                operation.type = ty;
                operation.refreshUid();
                if (!result) {
                    operation.attachedData = std::make_shared<EthereumOperationAttachedData>();
                    updateERC20Accounts(operation);
                }
                out.push_back(operation);
                operation.attachedData = nullptr;
            };

            if (_accountAddress == transaction.sender) {
                updateOperation(operation, api::OperationType::SEND);
                result = FLAG_TRANSACTION_CREATED_SENDING_OPERATION;
            }

            if (_accountAddress == transaction.receiver) {
                updateOperation(operation, api::OperationType::RECEIVE);
                result = FLAG_TRANSACTION_CREATED_RECEPTION_OPERATION;
            }

            // Case of parent transaction not belonging to account, but having side effect (transfer events)
            // concerning account address
            if (!result && (!transaction.erc20Transactions.empty() || !transaction.internalTransactions.empty())) {
                updateOperation(operation, api::OperationType::NONE);
            }
        }

        Try<int> EthereumLikeAccount::bulkInsert(const std::vector<Operation> &operations) {
            auto accountAddress = _accountAddress;
            return Try<int>::from([&] () {
                soci::session sql(getWallet()->getDatabase()->getPool());
                soci::transaction tr(sql);
                EthereumLikeOperationDatabaseHelper::bulkInsert(sql, operations, accountAddress);
                tr.commit();
                // Emit
                emitNewOperationsEvent(operations);
                for (const auto& op : operations) {
                    if (op.attachedData) {
                        const auto data = std::dynamic_pointer_cast<EthereumOperationAttachedData>(op.attachedData);
                        for (auto &it : data->erc20Operations) {
                            emitNewERC20Operation(std::get<1>(it), std::get<0>(it));
                        }
                    }
                }
                return operations.size();
            });
        }

        void EthereumLikeAccount::updateERC20Accounts(Operation &operation) {
            auto transaction = operation.ethereumTransaction.getValue();
            // No need filter because erc20 transfer events sent by explorer
            // are only the ones concerning current account
            if (!transaction.erc20Transactions.empty()) {
                for (auto &erc20Tx : transaction.erc20Transactions) {
                    erc20Tx.type = (erc20Tx.from == _accountAddress) ?
                                   api::OperationType::SEND : (erc20Tx.to == _accountAddress) ?
                                                              api::OperationType::RECEIVE : api::OperationType::NONE;

                    updateERC20Operation(operation, erc20Tx);
                    // Handle ERC20 self-transactions
                    if (erc20Tx.to == _accountAddress) {
                        erc20Tx.type = api::OperationType::RECEIVE;
                        updateERC20Operation(operation, erc20Tx);
                    }
                }
            }
        }

        void EthereumLikeAccount::updateERC20Operation(Operation &operation,
                                                       const ERC20Transaction &erc20Tx) {
            auto data = std::dynamic_pointer_cast<EthereumOperationAttachedData>(operation.attachedData);
            if (!data) {
                throw make_exception(api::ErrorCode::RUNTIME_ERROR, "Trying to interpret ERC20 without an attached data in operation");
            }
            auto erc20ContractAddress = erc20Tx.contractAddress;
            auto erc20OperationUid = OperationDatabaseHelper::createUid(operation.uid, erc20ContractAddress, erc20Tx.type);
            auto erc20Operation = std::make_shared<ERC20LikeOperation>(_accountAddress, erc20OperationUid, operation, erc20Tx, getWallet()->getCurrency());
            auto erc20AccountUid = AccountDatabaseHelper::createERC20AccountUid(getAccountUid(), erc20ContractAddress);

            //Check if account already exists
            auto needNewAccount = true;
            for (auto& account : _erc20LikeAccounts) {
                auto erc20Account = std::static_pointer_cast<ERC20LikeAccount>(account);
                if (erc20Account->getToken().contractAddress == erc20ContractAddress &&
                    erc20Account->getAddress() == _accountAddress) {
                    //Update account
                    erc20Account->putOperation(operation, erc20Operation);
                    needNewAccount = false;
                }

            }

            //Create a new account
            if (needNewAccount) {
                // TODO replace this
                auto erc20Token = api::ERC20Token("UNKNOWN_TOKEN", "UNKNOWN", erc20ContractAddress, 0);

                auto newAccount = std::make_shared<ERC20LikeAccount>(erc20AccountUid,
                                                                     erc20Token,
                                                                     _accountAddress,
                                                                     getWallet()->getCurrency(),
                                                                     std::dynamic_pointer_cast<EthereumLikeAccount>(shared_from_this()));
                data->accounts.push_back(newAccount);
                _erc20LikeAccounts.push_back(newAccount);
                //Persist erc20 account
                int erc20AccountCount = 0;
                newAccount->putOperation(operation, erc20Operation);
            }
        }

        std::vector<Operation> EthereumLikeAccount::getInternalOperations(soci::session &sql) {
            auto addr = _keychain->getAddress()->toString();

            soci::rowset<soci::row> rows = (sql.prepare <<
                "SELECT io.type, io.value, io.sender, io.receiver, io.gas_limit, io.gas_used, et.gas_price, op.date, et.status "
                "FROM internal_operations as io "
                "JOIN operations as op on io.ethereum_operation_uid = op.uid "
                "JOIN ethereum_operations as eo on eo.uid = op.uid "
                "JOIN ethereum_transactions as et on eo.transaction_uid = et.transaction_uid "
                "WHERE io.receiver = :addr OR io.sender = :addr",
                soci::use(addr, "addr")
            );

            std::vector<Operation> operations;

            for (auto& row : rows) {
                // ignore NONE operation
                Operation operation;

                operation.type = api::from_string<api::OperationType>(row.get<std::string>(0));

                if (operation.type == api::OperationType::NONE) {
                  continue;
                }

                //auto from = row.get<std::string>(2);
                //auto to = row.get<std::string>(3);
                auto gasLimit = BigInt::fromHex(row.get<std::string>(4));
                auto gasUsed = BigInt::fromHex(row.get<std::string>(5));
                auto gasPrice = BigInt::fromHex(row.get<std::string>(6));

                operation.date = DateUtils::fromJSON(row.get<std::string>(7));

                // we set fees to zero because they’re paid by the parent transaction if not set to NONE
                operation.fees = BigInt::ZERO;

                // if the status is not okay, we have to change the amount of the operation because
                // it wasn’t really broadcasted, but the fees were still paid
                auto status = soci::get_number<uint64_t>(row, 8);
                if (status == 0) {
                    operation.amount = BigInt::ZERO;
                } else {
                    operation.amount = BigInt::fromHex(row.get<std::string>(1));
                }

                // required when computing balances
                EthereumLikeBlockchainExplorerTransaction etx;
                etx.status = status;

                operation.ethereumTransaction = Option<EthereumLikeBlockchainExplorerTransaction>(etx);

                operations.push_back(operation);
            }

            return operations;
        }

        bool EthereumLikeAccount::putBlock(soci::session& sql,
                                           const EthereumLikeBlockchainExplorer::Block& block) {
                Block abstractBlock;
                abstractBlock.hash = block.hash;
                abstractBlock.currencyName = getWallet()->getCurrency().name;
                abstractBlock.height = block.height;
                abstractBlock.time = block.time;
                if (BlockDatabaseHelper::putBlock(sql, abstractBlock)) {
                        emitNewBlockEvent(abstractBlock);
                        return true;
                }
                return false;
        }

        std::shared_ptr<EthereumLikeKeychain> EthereumLikeAccount::getKeychain() const {
                return _keychain;
        }

        FuturePtr<Amount> EthereumLikeAccount::getBalance() {
            auto cachedBalance = getWallet()->getBalanceFromCache(getIndex());
            if (cachedBalance.hasValue()) {
                return FuturePtr<Amount>::successful(std::make_shared<Amount>(cachedBalance.getValue()));
            }
            std::vector<EthereumLikeKeychain::Address> listAddresses{_keychain->getAddress()};
            auto currency = getWallet()->getCurrency();
            auto self = getSelf();
            return _explorer->getBalance(listAddresses).mapPtr<Amount>(getMainExecutionContext(), [self, currency] (const std::shared_ptr<BigInt> &balance) -> std::shared_ptr<Amount> {
                Amount b(currency, 0, BigInt(balance->toString()));
                self->getWallet()->updateBalanceCache(self->getIndex(), b);
                return std::make_shared<Amount>(b);
            });
        }

        std::shared_ptr<api::OperationQuery> EthereumLikeAccount::queryOperations() {
            auto query = std::make_shared<OperationQuery>(
                    api::QueryFilter::accountEq(getAccountUid()),
                    getWallet()->getDatabase(),
                    getWallet()->getPool()->getThreadPoolExecutionContext(),
                    getMainExecutionContext()
            );
            query->registerAccount(shared_from_this());
            return query;
        }

        Future<AbstractAccount::AddressList> EthereumLikeAccount::getFreshPublicAddresses() {
                auto keychain = getKeychain();
                return async<AbstractAccount::AddressList>([=] () -> AbstractAccount::AddressList {
                    AbstractAccount::AddressList result{keychain->getAddress()};
                    return result;
                });
        }

        Future<std::vector<std::shared_ptr<api::Amount>>>
        EthereumLikeAccount::getBalanceHistory(const std::string & start,
                                               const std::string & end,
                                               api::TimePeriod precision) {
                auto self = getSelf();
                return Future<std::vector<std::shared_ptr<api::Amount>>>::async(getWallet()->getPool()->getThreadPoolExecutionContext(), [=] () -> std::vector<std::shared_ptr<api::Amount>> {
                    auto startDate = DateUtils::fromJSON(start);
                    auto endDate = DateUtils::fromJSON(end);
                    if (startDate >= endDate) {
                        throw make_exception(api::ErrorCode::INVALID_DATE_FORMAT,
                                             "Start date should be strictly lower than end date");
                    }

                    const auto &uid = self->getAccountUid();
                    soci::session sql(self->getWallet()->getDatabase()->getReadonlyPool());
                    std::vector<Operation> operations;

                    auto keychain = self->getKeychain();
                    std::function<bool(const std::string &)> filter = [&keychain](const std::string& addr) -> bool {
                        auto keychainAddr = keychain->getAddress()->toString();
                        return addr == keychainAddr;
                    };

                    // Get operations related to an account
                    OperationDatabaseHelper::queryOperations(sql, uid, operations, filter);

                    // Get internal operations, add them to the list of operations and deallocate
                    // them to free memory
                    {
                        auto internalOperations = getInternalOperations(sql);

                        // add the internal operations to the list of operations
                        operations.insert(operations.end(), internalOperations.begin(), internalOperations.end());
                    }

                    // sort operations
                    std::sort(operations.begin(), operations.end(), [](Operation const& a, Operation const& b) {
                        return a.date < b.date;
                    });

                    auto lowerDate = startDate;
                    auto upperDate = DateUtils::incrementDate(startDate, precision);

                    std::vector<std::shared_ptr<api::Amount>> amounts;
                    std::size_t operationsCount = 0;
                    BigInt sum;
                    while (lowerDate <= endDate && operationsCount < operations.size()) {
                        auto operation = operations[operationsCount];

                        while (operation.date > upperDate && lowerDate < endDate) {
                            lowerDate = DateUtils::incrementDate(lowerDate, precision);
                            upperDate = DateUtils::incrementDate(upperDate, precision);
                            amounts.emplace_back(
                                    std::make_shared<ledger::core::Amount>(self->getWallet()->getCurrency(), 0, sum));
                        }

                        if (operation.date <= upperDate) {
                            switch (operation.type) {
                                case api::OperationType::RECEIVE: {
                                    sum = sum + operation.amount;
                                    break;
                                }

                                case api::OperationType::SEND: {
                                    sum = sum - (operation.amount + operation.fees.getValueOr(BigInt::ZERO));
                                    break;
                                }

                                default:
                                    break;
                            }
                        }

                        operationsCount += 1;
                    }

                    while (lowerDate < endDate) {
                        lowerDate = DateUtils::incrementDate(lowerDate, precision);

                        amounts.emplace_back(
                            std::make_shared<ledger::core::Amount>(self->getWallet()->getCurrency(), 0, sum)
                        );
                    }

                    return amounts;
                });
        }

        Future<api::ErrorCode> EthereumLikeAccount::eraseDataSince(const std::chrono::system_clock::time_point & date) {
                auto log = logger();

                log->debug(" Start erasing data of account : {}", getAccountUid());

                std::lock_guard<std::mutex> lock(_synchronizationLock);
                _currentSyncEventBus = nullptr;

                soci::session sql(getWallet()->getDatabase()->getPool());

                //Update account's internal preferences (for synchronization)
                // Clear synchronizer state
                eraseSynchronizerDataSince(sql, date);

                auto accountUid = getAccountUid();
                EthereumLikeTransactionDatabaseHelper::eraseDataSince(sql, accountUid, date);
                return Future<api::ErrorCode>::successful(api::ErrorCode::FUTURE_WAS_SUCCESSFULL);

        }

        bool EthereumLikeAccount::isSynchronizing() {
                std::lock_guard<std::mutex> lock(_synchronizationLock);
                return _currentSyncEventBus != nullptr;
        }

        std::shared_ptr<api::EventBus> EthereumLikeAccount::synchronize() {
            std::lock_guard<std::mutex> lock(_synchronizationLock);
            if (_currentSyncEventBus)
                    return _currentSyncEventBus;
            auto eventPublisher = std::make_shared<EventPublisher>(getContext());

            _currentSyncEventBus = eventPublisher->getEventBus();
            auto future = _synchronizer->synchronize(std::static_pointer_cast<EthereumLikeAccount>(shared_from_this()))->getFuture();
            auto self = std::static_pointer_cast<EthereumLikeAccount>(shared_from_this());

            //Update current block height (needed to compute trust level)
            _explorer->getCurrentBlock().onComplete(getContext(), [self] (const TryPtr<EthereumLikeBlockchainExplorer::Block>& block) mutable {
                if (block.isSuccess()) {
                    self->_currentBlockHeight = block.getValue()->height;
                    soci::session sql(self->getWallet()->getDatabase()->getPool());
                    BulkInsertDatabaseHelper::updateBlock(sql, *block.getValue());
                }
            });

            auto startTime = DateUtils::now();
            eventPublisher->postSticky(std::make_shared<Event>(api::EventCode::SYNCHRONIZATION_STARTED, api::DynamicObject::newInstance()), 0);
            future.onComplete(getContext(), [eventPublisher, self, startTime] (const auto& result) {
                api::EventCode code;
                auto payload = std::make_shared<DynamicObject>();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(DateUtils::now() - startTime).count();
                payload->putLong(api::Account::EV_SYNC_DURATION_MS, duration);
                if (result.isSuccess()) {
                    code = api::EventCode::SYNCHRONIZATION_SUCCEED;

                    auto const context = result.getValue();

                    payload->putInt(api::Account::EV_SYNC_LAST_BLOCK_HEIGHT, static_cast<int32_t>(context.lastBlockHeight));
                    payload->putInt(api::Account::EV_SYNC_NEW_OPERATIONS, static_cast<int32_t>(context.newOperations));

                    if (context.reorgBlockHeight) {
                        payload->putInt(api::Account::EV_SYNC_REORG_BLOCK_HEIGHT, static_cast<int32_t>(context.reorgBlockHeight.getValue()));
                    }
                } else {
                    code = api::EventCode::SYNCHRONIZATION_FAILED;
                    payload->putString(api::Account::EV_SYNC_ERROR_CODE, api::to_string(result.getFailure().getErrorCode()));
                    payload->putInt(api::Account::EV_SYNC_ERROR_CODE_INT, (int32_t)result.getFailure().getErrorCode());
                    payload->putString(api::Account::EV_SYNC_ERROR_MESSAGE, result.getFailure().getMessage());
                }
                eventPublisher->postSticky(std::make_shared<Event>(code, payload), 0);
                std::lock_guard<std::mutex> lock(self->_synchronizationLock);
                self->_currentSyncEventBus = nullptr;
            });
            return eventPublisher->getEventBus();
        }

        std::shared_ptr<EthereumLikeAccount> EthereumLikeAccount::getSelf() {
                return std::dynamic_pointer_cast<EthereumLikeAccount>(shared_from_this());
        }

        std::string EthereumLikeAccount::getRestoreKey() {
                return _keychain->getRestoreKey();
        }

        EthereumLikeBlockchainExplorerTransaction EthereumLikeAccount::getETHLikeBlockchainExplorerTxFromRawTx(const std::shared_ptr<EthereumLikeAccount> &account,
                                                                                                               const std::string &txHash,
                                                                                                               const std::vector<uint8_t> &rawTx) {
            auto tx = EthereumLikeTransactionBuilder::parseRawSignedTransaction(account->getWallet()->getCurrency(), rawTx);
            EthereumLikeBlockchainExplorerTransaction txExplorer;
            // It is an optimistic so it should be successful (but tx could fail e.g. out of gas error but it will be updated when sync again )
            auto sender = account->getKeychain()->getAddress()->toString();
            txExplorer.status = 1;
            txExplorer.hash = txHash;
            txExplorer.gasLimit = BigInt(tx->getGasLimit()->toString());
            txExplorer.gasPrice = BigInt(tx->getGasPrice()->toString());
            txExplorer.gasUsed = BigInt::ZERO; // Tx is not mined yet so ...(This will updated at next synchro)
            txExplorer.value = BigInt(tx->getValue()->toString());
            txExplorer.sender = sender;
            txExplorer.receiver = tx->getReceiver()->toEIP55();
            txExplorer.receivedAt = std::chrono::system_clock::now();
            txExplorer.inputData = tx->getData().value_or(std::vector<uint8_t>());
            // Create ERC20 Ops
            auto strInputData = hex::toString(txExplorer.inputData);
            // 136 / 2 => 68 bytes = 4 bytes for transfer method ID (0xa9059cbb) + 32 bytes for receiver address + 32 bytes for amount
            if (strInputData.size() == 136 && strInputData.find(erc20Tokens::ERC20MethodsID.at("transfer")) != std::string::npos) {
                ERC20Transaction erc20Tx;

                erc20Tx.from = sender;

                BytesReader reader(txExplorer.inputData);
                reader.read(hex::toByteArray(erc20Tokens::ERC20MethodsID.at("transfer")).size());

                // Get rid of leading zeros
                auto skipEIP55Check = true;
                //auto toAddress = BigInt::fromHex(hex::toString(reader.read(32))).toHexString();
                erc20Tx.to = EthereumLikeAddress::fromEIP55(
                        "0x" + BigInt::fromHex(hex::toString(reader.read(32))).toHexString(),
                        account->getWallet()->getCurrency(), Option<std::string>(""), skipEIP55Check)
                        ->toEIP55();
                erc20Tx.value = BigInt::fromHex(hex::toString(reader.read(32)));
                erc20Tx.type = api::OperationType::SEND;
                erc20Tx.contractAddress = tx->getReceiver()->toEIP55();
                txExplorer.erc20Transactions.push_back(erc20Tx);
            }
            return txExplorer;
        }

        void EthereumLikeAccount::broadcastRawTransaction(const std::vector<uint8_t> & transaction,
                                                          const std::shared_ptr<api::StringCallback> & callback) {
            auto self = getSelf();
            _explorer->pushTransaction(transaction).map<std::string>(getContext(), [self, transaction] (const String& seq) -> std::string {
                auto txHash = seq.str();
                auto optimisticUpdate = Try<int>::from([&] () -> int {
                    auto txExplorer = getETHLikeBlockchainExplorerTxFromRawTx(self, txHash, transaction);
                    //Store in DB
                    std::vector<Operation> operations;
                    self->interpretTransaction(txExplorer, operations);
                    self->bulkInsert(operations);
                    self->emitEventsNow();
                    return operations.size();
                });

                return txHash;
            }).callback(getMainExecutionContext(), callback);
        }

        void EthereumLikeAccount::broadcastTransaction(const std::shared_ptr<api::EthereumLikeTransaction> & transaction,
                                                       const std::shared_ptr<api::StringCallback> & callback) {
                broadcastRawTransaction(transaction->serialize(), callback);
        }

        std::shared_ptr<api::EthereumLikeAccount> EthereumLikeAccount::asEthereumLikeAccount() {
                return std::dynamic_pointer_cast<EthereumLikeAccount>(shared_from_this());
        }

        std::vector<std::shared_ptr<api::ERC20LikeAccount>>
        EthereumLikeAccount::getERC20Accounts() {
                return _erc20LikeAccounts;
        }

        void EthereumLikeAccount::getGasPrice(const std::shared_ptr<api::BigIntCallback> & callback) {
            _explorer->getGasPrice().mapPtr<api::BigInt>(getMainExecutionContext(), [] (const std::shared_ptr<BigInt> &gasPrice) -> std::shared_ptr<api::BigInt> {
                return std::make_shared<api::BigIntImpl>(*gasPrice);
            }).callback(getMainExecutionContext(), callback);
        }

        void EthereumLikeAccount::getEstimatedGasLimit(const std::string & address, const std::shared_ptr<api::BigIntCallback> & callback) {
            _explorer->getEstimatedGasLimit(address).mapPtr<api::BigInt>(getMainExecutionContext(), [] (const std::shared_ptr<BigInt> &gasPrice) -> std::shared_ptr<api::BigInt> {
                return std::make_shared<api::BigIntImpl>(*gasPrice);
            }).callback(getMainExecutionContext(), callback);
        }

        void EthereumLikeAccount::getDryRunGasLimit(const std::string & address, const api::EthereumGasLimitRequest &request, const std::shared_ptr<api::BigIntCallback> & callback) {
            _explorer->getDryRunGasLimit(address, request).mapPtr<api::BigInt>(getMainExecutionContext(), [] (const std::shared_ptr<BigInt> &gasPrice) -> std::shared_ptr<api::BigInt> {
                return std::make_shared<api::BigIntImpl>(*gasPrice);
            }).callback(getMainExecutionContext(), callback);
        }

        FuturePtr<api::BigInt> EthereumLikeAccount::getERC20Balance(const std::string & erc20Address) {
            return _explorer->getERC20Balance(_keychain->getAddress()->toEIP55(), erc20Address).mapPtr<api::BigInt>(getMainExecutionContext(), [] (const std::shared_ptr<BigInt> &erc20Balance) -> std::shared_ptr<api::BigInt> {
                return std::make_shared<api::BigIntImpl>(*erc20Balance);
            });
        }

        void EthereumLikeAccount::getERC20Balance(const std::string & erc20Address,
                                                  const std::shared_ptr<api::BigIntCallback> & callback) {
            getERC20Balance(erc20Address).callback(getMainExecutionContext(), callback);
        }

        Future<std::vector<std::shared_ptr<api::BigInt>>> EthereumLikeAccount::getERC20Balances(const std::vector<std::string> &erc20Addresses) {
            return _explorer->getERC20Balances(_keychain->getAddress()->toEIP55(), erc20Addresses)
                    .map<std::vector<std::shared_ptr<api::BigInt>>>(getMainExecutionContext(),
                                                                    [] (const std::vector<BigInt> &erc20Balances) {
                                                                        return vector::map<std::shared_ptr<api::BigInt>, BigInt>(erc20Balances, [] (const BigInt &erc20Balance) {
                                                                            return std::make_shared<api::BigIntImpl>(erc20Balance);
                                                                        });
                                                                    }
                    );
        }

        void EthereumLikeAccount::getERC20Balances(const std::vector<std::string> &erc20Addresses, const std::shared_ptr<api::BigIntListCallback> & callback) {
            getERC20Balances(erc20Addresses).callback(getMainExecutionContext(), callback);
        }

        void EthereumLikeAccount::addERC20Accounts(soci::session &sql,
                                                   const std::vector<ERC20LikeAccountDatabaseEntry> &erc20Entries) {
            auto self = std::dynamic_pointer_cast<EthereumLikeAccount>(shared_from_this());
            for (auto &erc20Entry : erc20Entries) {
                auto erc20Token = EthereumLikeAccountDatabaseHelper::getOrCreateERC20Token(sql, erc20Entry.contractAddress);
                auto newERC20Account = std::make_shared<ERC20LikeAccount>(erc20Entry.uid,
                                                                          erc20Token,
                                                                          self->getKeychain()->getAddress()->toEIP55(),
                                                                          self->getWallet()->getCurrency(),
                                                                          self);
                _erc20LikeAccounts.push_back(newERC20Account);
            }
        }

        std::shared_ptr<api::EthereumLikeTransactionBuilder> EthereumLikeAccount::buildTransaction() {
                auto self = std::dynamic_pointer_cast<EthereumLikeAccount>(shared_from_this());
                auto buildFunction = [self] (const EthereumLikeTransactionBuildRequest& request, const std::shared_ptr<EthereumLikeBlockchainExplorer> &explorer) -> Future<std::shared_ptr<api::EthereumLikeTransaction>> {
                    // Check if balance is sufficient
                    return self->getBalance().flatMapPtr<api::EthereumLikeTransaction>(self->getMainExecutionContext(), [self, request, explorer](const std::shared_ptr<Amount> &balance) {
                        // Check if all needed values are set
                        if (!request.gasLimit || !request.gasPrice || (!request.value && !request.wipe)) {
                            throw make_exception(api::ErrorCode::INVALID_ARGUMENT, "Missing mandatory informations (e.g. gasLimit, gasPrice or value).");
                        }
                        // Check for balance
                        auto maxPossibleAmountToSend = BigInt(balance->toString()) - *(request.gasLimit) * *(request.gasPrice);
                        auto amountToSend = request.wipe ? BigInt::ZERO : *request.value;
                        if (maxPossibleAmountToSend < amountToSend) {
                            throw make_exception(api::ErrorCode::NOT_ENOUGH_FUNDS, "Cannot gather enough funds.");
                        }
                        auto tx = std::make_shared<EthereumLikeTransactionApi>(self->getWallet()->getCurrency());
                        tx->setValue(request.wipe ? std::make_shared<BigInt>(maxPossibleAmountToSend) : request.value);
                        tx->setData(request.inputData);
                        tx->setGasLimit(request.gasLimit);
                        tx->setGasPrice(request.gasPrice);
                        tx->setReceiver(request.toAddress);
                        auto accountAddress = self->getKeychain()->getAddress()->toString();
                        tx->setSender(accountAddress);
                        return explorer->getNonce(accountAddress).map<std::shared_ptr<api::EthereumLikeTransaction>>(self->getMainExecutionContext(), [self, tx] (const std::shared_ptr<BigInt> &nonce) -> std::shared_ptr<api::EthereumLikeTransaction> {
                            tx->setNonce(nonce);
                            return tx;
                        });
                    });
                };

                return std::make_shared<EthereumLikeTransactionBuilder>(getMainExecutionContext(),
                                                                        getWallet()->getCurrency(),
                                                                        _explorer,
                                                                        logger(),
                                                                        buildFunction);
        }

        std::shared_ptr<api::Keychain> EthereumLikeAccount::getAccountKeychain() {
            return _keychain;
        }

        void EthereumLikeAccount::emitNewERC20Operation(ERC20LikeOperation& op, const std::string &accountUid) {
           std::vector<ERC20LikeOperation> ops {op};
           emitNewERC20Operations(ops, accountUid);
        }

        void EthereumLikeAccount::emitEventsNow() {
            if (_batchedErc20Event) {
                std::unique_lock<std::mutex> lock(_erc20EventLock);
                pushEvent(_batchedErc20Event);
                _batchedErc20Event = nullptr;
            }
            AbstractAccount::emitEventsNow();
        }

        void EthereumLikeAccount::emitNewERC20Operations(std::vector<ERC20LikeOperation> &ops,
                                                         const std::string &accountUid) {
            if (ops.empty()) {
                return ;
            }
            std::unique_lock<std::mutex> lock(_erc20EventLock);
            if (!_batchedErc20Event) {
                _batchedErc20Event = Event::newInstance(api::EventCode::UPDATE_ERC20_OPERATIONS, DynamicObject::newInstance());
                std::dynamic_pointer_cast<core::Event>(_batchedErc20Event)->setReadOnly(false);
                _batchedErc20Event->getPayload()->putArray(api::Account::EV_NEW_OP_UID, DynamicArray::newInstance());
                _batchedErc20Event->getPayload()->putArray(api::ERC20LikeAccount::EV_NEW_OP_ERC20_ACCOUNT_UID, DynamicArray::newInstance());
                _batchedErc20Event->getPayload()->putString(api::Account::EV_NEW_OP_WALLET_NAME, getWallet()->getName());
                _batchedErc20Event->getPayload()->putLong(api::Account::EV_NEW_OP_ACCOUNT_INDEX, getIndex());
            }
            for (auto& op : ops) {
                _batchedErc20Event
                        ->getPayload()
                        ->getArray(api::Account::EV_NEW_OP_UID)
                        ->pushString(op.getOperationUid());
                _batchedErc20Event
                    ->getPayload()
                    ->getArray(api::ERC20LikeAccount::EV_NEW_OP_ERC20_ACCOUNT_UID)
                    ->pushString(accountUid);
            }
        }


    }
}
