/*
 * Copyright 2004       Martin Preuss <martin@libchipcard.de>
 * Copyright 2004-2019  Thomas Baumgart <tbaumgart@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 * @short A C++ wrapper of the main aqbanking interface
 */

#ifndef AQ_BANKING_CPP_H
#define AQ_BANKING_CPP_H

#include <aqbanking/version.h>
#ifndef AQB_MAKE_VERSION
#define AQB_MAKE_VERSION(a,b,c,d) (((a)<<24) | ((b)<<16) | (c<<8) | (d))
#endif

#ifndef AQBANKING_VERSION
#define AQBANKING_VERSION AQB_MAKE_VERSION(AQBANKING_VERSION_MAJOR,AQBANKING_VERSION_MINOR,AQBANKING_VERSION_PATCHLEVEL,AQBANKING_VERSION_BUILD)
#endif

#ifndef AQB_IS_VERSION
#define AQB_IS_VERSION(a,b,c,d) (AQBANKING_VERSION >= AQB_MAKE_VERSION(a,b,c,d))
#endif

#if AQB_IS_VERSION(5,99,0,0)
#include <aqbanking/types/account_spec.h>
typedef AB_ACCOUNT_SPEC AB_ACCOUNT;
#define AB_Account_free AB_AccountSpec_free
#define AB_Account_GetUniqueId AB_AccountSpec_GetUniqueId
#define AB_Account_GetBankCode AB_AccountSpec_GetBankCode
#define AB_Account_GetAccountNumber AB_AccountSpec_GetAccountNumber
#define AB_Account_GetProvider AB_AccountSpec_GetBackendName
#define AB_Account_GetAccountName AB_AccountSpec_GetAccountName
#define AB_Account_GetBankName(a) ""
#define AB_Account_GetOwnerName AB_AccountSpec_GetOwnerName
#define AB_Account_GetIBAN AB_AccountSpec_GetIban
#define AB_Account_GetBIC  AB_AccountSpec_GetBic

#define AB_Banking_GetAccountByAlias AB_Banking_GetAccountSpecByAlias
#define AB_Banking_ExecuteJobs AB_Banking_SendCommands

#define AB_ImExporterAccountInfo_GetType AB_ImExporterAccountInfo_GetAccountType
#define AB_ImporterDialog_new AB_Banking_CreateImporterDialog
#define AB_Provider_GetName(a) a

typedef AB_TRANSACTION AB_JOB;
#include <aqbanking/types/transaction.h>
#define AB_Job_Attach AB_Transaction_Attach
static AB_ACCOUNT_SPEC *AB_Job_GetAccount(AB_JOB *a) { return (AB_ACCOUNT_SPEC*)1; }
#define AB_Job_GetCreatedBy(a) 0
#define AB_Job_GetJobId AB_Transaction_GetIdForApplication
#define AB_Job_GetStatus (AB_JOB_STATUS)AB_Transaction_GetStatus
#define AB_Job_GetType AB_Transaction_GetCommand
#define AB_Job_free AB_Transaction_free

#define AB_JOB_LIST2 AB_TRANSACTION_LIST2
#define AB_JOB_LIST2_ITERATOR AB_TRANSACTION_LIST2_ITERATOR
#define AB_Job_List2_First AB_Transaction_List2_First
#define AB_Job_List2_FreeAll AB_Transaction_List2_freeAll
#define AB_Job_List2_GetSize AB_Transaction_List2_GetSize
#define AB_Job_List2_new AB_Transaction_List2_new
#define AB_Job_List2_Remove AB_Transaction_List2_Remove

#define AB_Job_List2Iterator_Next AB_Transaction_List2Iterator_Next
#define AB_Job_List2Iterator_free AB_Transaction_List2Iterator_free
#define AB_Job_List2Iterator_Data AB_Transaction_List2Iterator_Data
#define AB_Job_List2_PushBack AB_Transaction_List2_PushBack

typedef enum {
  AB_Job_StatusNew = AB_Transaction_StatusUnknown,
  AB_Job_StatusUpdated = AB_Transaction_StatusNone,
  AB_Job_StatusEnqueued = AB_Transaction_StatusEnqueued,
  AB_Job_StatusSent = AB_Transaction_StatusSent,
  AB_Job_StatusPending = AB_Transaction_StatusPending,
  AB_Job_StatusFinished = AB_Transaction_StatusAccepted,
  AB_Job_StatusError =  AB_Transaction_StatusError,
  AB_Job_StatusUnknown =  AB_Transaction_StatusUnknown,
} AB_JOB_STATUS;

/*
typedef enum {
  AB_Transaction_StatusUnknown = -1,
  AB_Transaction_StatusNone = 0,
  AB_Transaction_StatusEnqueued,
  AB_Transaction_StatusSending,
  AB_Transaction_StatusSent,
  AB_Transaction_StatusAccepted,
  AB_Transaction_StatusRejected,
  AB_Transaction_StatusPending,
  AB_Transaction_StatusAutoReconciled,
  AB_Transaction_StatusManuallyReconciled,
  AB_Transaction_StatusRevoked,
  AB_Transaction_StatusAborted,
  AB_Transaction_StatusError
} AB_TRANSACTION_STATUS;
*/

typedef enum {
  AB_Job_TypeUnknown                = AB_Transaction_CommandUnknown,
  AB_Job_TypeGetBalance             = AB_Transaction_CommandGetBalance,
  AB_Job_TypeGetTransactions        = AB_Transaction_CommandGetTransactions,
  AB_Job_TypeTransfer               = AB_Transaction_CommandTransfer,
  AB_Job_TypeDebitNote              = AB_Transaction_CommandDebitNote,
  //AB_Job_TypeEuTransfer             = AB_Transaction_CommandEuTransfer,
  AB_Job_TypeGetStandingOrders      = AB_Transaction_CommandGetStandingOrders,
  AB_Job_TypeGetDatedTransfers      = AB_Transaction_CommandGetDatedTransfers,
  AB_Job_TypeCreateStandingOrder    = AB_Transaction_CommandCreateStandingOrder,
  AB_Job_TypeModifyStandingOrder    = AB_Transaction_CommandModifyStandingOrder,
  AB_Job_TypeDeleteStandingOrder    = AB_Transaction_CommandDeleteStandingOrder,
  AB_Job_TypeCreateDatedTransfer    = AB_Transaction_CommandCreateDatedTransfer,
  AB_Job_TypeModifyDatedTransfer    = AB_Transaction_CommandModifyDatedTransfer,
  AB_Job_TypeDeleteDatedTransfer    = AB_Transaction_CommandDeleteDatedTransfer,
  AB_Job_TypeInternalTransfer       = AB_Transaction_CommandInternalTransfer,
  AB_Job_TypeLoadCellPhone          = AB_Transaction_CommandLoadCellPhone,
  AB_Job_TypeSepaTransfer           = AB_Transaction_CommandSepaTransfer,
  AB_Job_TypeSepaDebitNote          = AB_Transaction_CommandSepaDebitNote,
  AB_Job_TypeSepaCreateStandingOrder = AB_Transaction_CommandSepaCreateStandingOrder,
  AB_Job_TypeSepaModifyStandingOrder = AB_Transaction_CommandSepaModifyStandingOrder,
  AB_Job_TypeSepaDeleteStandingOrder = AB_Transaction_CommandSepaDeleteStandingOrder,
  AB_Job_TypeSepaFlashDebitNote     = AB_Transaction_CommandSepaFlashDebitNote,
  AB_Job_TypeSepaGetStandingOrders  = AB_Transaction_CommandSepaGetStandingOrders,
} AB_JOB_TYPE;

#define AB_JobGetTransactions_GetMaxStoreDays(a) (0)

#define AB_SetupDialog_new AB_Banking_CreateSetupDialog
#define AB_Transaction_GetOriginatorIdentifier AB_Transaction_GetOriginatorId
#define AB_TransactionLimits_GetMaxLinesRemoteName(aqlimits) 1
#define GWEN_StringList_fromQString(a) a.toUtf8().constData()

#include <aqbanking/types/imexporter_context.h>
#include <aqbanking/types/transactionlimits.h>
#include <aqbanking/types/value.h>
#include <aqbanking/gui/abgui.h>
#else
#include <aqbanking/account.h>
#include <aqbanking/imexporter.h>
#include <aqbanking/jobsingletransfer.h>
#include <aqbanking/jobsepatransfer.h>
#include <aqbanking/jobgettransactions.h>
#include <aqbanking/jobgetbalance.h>
#include <aqbanking/job.h>
#include <aqbanking/abgui.h>
#include <aqbanking/dlg_setup.h>
#include <aqbanking/dlg_importer.h>
#include <aqbanking/transactionlimits.h>
#include <aqbanking/transaction.h>
#include <aqbanking/value.h>
#endif

#include <aqbanking/banking.h>
#include <aqbanking/system.h>

#include <list>
#include <string>


/**
 * @brief A C++ binding for the C module @ref AB_BANKING
 *
 * This class simply is a C++ binding for the C module @ref AB_BANKING.
 * It redirects C callbacks used by AB_BANKING to virtual functions in
 * this class. It als transforms some return values inconveniant for
 * C++ into STL objects (such as "list<T>").
 *
 * @ingroup G_AB_CPP_INTERFACE
 *
 * @author Martin Preuss<martin@aquamaniac.de>
 */
class AB_Banking {
private:
  AB_BANKING *_banking;

public:
  AB_Banking(const char *appname,
          const char *fname);
  virtual ~AB_Banking();


  AB_BANKING *getCInterface();


  /**
   * See @ref AB_Banking_Init
   */
  virtual int init();

  /**
   * See @ref AB_Banking_Fini
   */
  virtual int fini();


#if !AQB_IS_VERSION(5,99,0,0)
  /**
   * See @ref AB_Banking_OnlineInit
   */
  int onlineInit();

  /**
   * See @ref AB_Banking_OnlineFini
   */
  int onlineFini();

  /**
   * Loads a backend with the given name. You can use
   * @ref AB_Banking_GetProviderDescrs to retrieve a list of available
   * backends. Such a backend can then be asked to return an account list.
   */
  AB_PROVIDER *getProvider(const char *name);
#endif
  /**
   * TODO
   */
  std::list<std::string> getActiveProviders();

  /**
   * Returns the application name as given to @ref AB_Banking_new.
   */
  const char *getAppName();

  /**
   * Returns a list of pointers to currently known accounts.
   * Please note that the pointers in this list are still owned by
   * AqBanking, so you MUST NOT free them.
   * However, destroying the list will not free the accounts, so it is
   * safe to do that.
   */
#if AQB_IS_VERSION(5,99,0,0)
  std::list<AB_ACCOUNT_SPEC*> getAccounts();
#else
  std::list<AB_ACCOUNT*> getAccounts();
#endif

  /**
   * This function does an account lookup based on the given unique id.
   * This id is assigned by AqBanking when an account is created.
   * The pointer returned is still owned by AqBanking, so you MUST NOT free
   * it.
   */
#if AQB_IS_VERSION(5,99,0,0)
  AB_ACCOUNT_SPEC *getAccount(uint32_t uniqueId);
#else
  AB_ACCOUNT *getAccount(uint32_t uniqueId);

  /**
   * Returns a list of pointers to currently known users.
   * Please note that the pointers in this list are still owned by
   * AqBanking, so you MUST NOT free them.
   * However, destroying the list will not free the users, so it is
   * safe to do that.
   */
  std::list<AB_USER*> getUsers();
#endif

  int getUserDataDir(GWEN_BUFFER *buf) const ;
#if !AQB_IS_VERSION(5,99,0,0)
  int getAppUserDataDir(GWEN_BUFFER *buf) const ;

  int loadAppConfig(GWEN_DB_NODE **pDb);
  int saveAppConfig(GWEN_DB_NODE *db);
  int lockAppConfig();
  int unlockAppConfig();

  int loadAppSubConfig(const char *subGroup,
                      GWEN_DB_NODE **pDb);

  int saveAppSubConfig(const char *subGroup,
                      GWEN_DB_NODE *dbSrc);
#endif

  int loadSharedConfig(const char *name, GWEN_DB_NODE **pDb);
  int saveSharedConfig(const char *name, GWEN_DB_NODE *db);
  int lockSharedConfig(const char *name);
  int unlockSharedConfig(const char *name);

  int loadSharedSubConfig(const char *name,
                         const char *subGroup,
                         GWEN_DB_NODE **pDb);

  int saveSharedSubConfig(const char *name,
                         const char *subGroup,
                         GWEN_DB_NODE *dbSrc);

#if !AQB_IS_VERSION(5,99,0,0)
  int beginExclUseAccount(AB_ACCOUNT *a);
  int endExclUseAccount(AB_ACCOUNT *a, int abandon);

  int beginExclUseUser(AB_USER *u);
  int endExclUseUser(AB_USER *u, int abandon);

  void setAccountAlias(AB_ACCOUNT *a, const char *alias);
#else
  void setAccountAlias(AB_ACCOUNT_SPEC *a, const char *alias);
#endif

  /**
   * Provide interface to setup ZKA FinTS registration
   */
  void registerFinTs(const char* regKey, const char* version) const;

  /** @name Enqueueing, Dequeueing and Executing Jobs
   *
   * Enqueued jobs are preserved across shutdowns. As soon as a job has been
   * sent to the appropriate backend it will be removed from the queue.
   * Only those jobs are saved/reloaded which have been enqueued but never
   * presented to the backend. This means after calling
   * @ref AB_Banking_ExecuteQueue only those jobs are still in the queue which
   * have not been processed (e.g. because they belonged to a second backend
   * but the user aborted while the jobs for a first backend were in process).
   */
  /*@{*/
  /**
   * This function sends all jobs in the list to their corresponding backends
   * and allows that backend to process it.
   */
#if AQB_IS_VERSION(5,99,0,0)
  virtual int executeJobs(AB_TRANSACTION_LIST2 *jl,
#else
  virtual int executeJobs(AB_JOB_LIST2 *jl,
#endif
                         AB_IMEXPORTER_CONTEXT *ctx);

  /*@}*/

  /**
   * Let the application import a given statement context.
   */
  virtual bool importContext(AB_IMEXPORTER_CONTEXT *ctx,
                             uint32_t flags);

  virtual bool importAccountInfo(AB_IMEXPORTER_ACCOUNTINFO *ai, uint32_t flags);

};




#endif /* AQ_BANKING_CPP_H */


