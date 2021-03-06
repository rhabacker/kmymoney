############# next target (views) STATIC ###################
set(libviews_a_SOURCES
  kaccountsview.cpp
  kbudgetview.cpp
  kcategoriesview.cpp
  kforecastview.cpp
  kgloballedgerview.cpp
  ledgerview.cpp
  kwelcomepage.cpp
  khomeview.cpp
  kinstitutionsview.cpp
  kinvestmentview.cpp
  kmymoneyfile.cpp
  kmymoneyview.cpp
  kpayeesview.cpp
  kreportsview.cpp
  kscheduledview.cpp
  tocitem.cpp
  tocitemgroup.cpp
  tocitemreport.cpp
  kscheduletreeitem.cpp
  ktagsview.cpp
  konlinejoboutbox.cpp
  kpayeeidentifierview.cpp
  payeeidentifierselectiondelegate.cpp
  simpleledgerview.cpp
  ledgerviewpage.cpp
  ledgerview.cpp
  ledgerdelegate.cpp
  newspliteditor.cpp
  newtransactioneditor.cpp
  newtransactionform.cpp
  splitdialog.cpp
  splitdelegate.cpp
  widgethintframe.cpp
)

set(libviews_a_HEADER kmymoneyview.h)

set(libviews_a_UI
  kaccountsviewdecl.ui
  kbudgetviewdecl.ui
  kcategoriesviewdecl.ui
  kforecastviewdecl.ui
  kinstitutionsviewdecl.ui
  kinvestmentviewdecl.ui
  kpayeesviewdecl.ui
  kscheduledviewdecl.ui
  ktagsviewdecl.ui
  konlinejoboutbox.ui
  kpayeeidentifierview.ui
  simpleledgerview.ui
  ledgerview.ui
  ledgerviewpage.ui
  simpleledgerview.ui
  splitdialog.ui
  newspliteditor.ui
  newtransactioneditor.ui
  newtransactionform.ui
)

# The handling of these ui files depends
# on libkmymoney.so (the widgets library)
ki18n_wrap_ui(libviews_a_SOURCES ${libviews_a_UI})

add_library(views STATIC ${libviews_a_SOURCES})

target_link_libraries(views PUBLIC KChart KF5::KIOFileWidgets KF5::Notifications KF5::Archive KF5::TextWidgets Qt5::Sql Qt5::WebEngineWidgets Alkimia::alkimia)

#to be deleted when QWebEngine 5.8 will be better spread across distros
if(KF5KHtml_FOUND)
  target_link_libraries(views PRIVATE KF5::KHtml)
endif()

if(KF5Activities_FOUND)
  target_link_libraries(views PRIVATE KF5::Activities)
endif()

# TODO: Remove this dependency. But it is needed as long as the payee editor uses these objects directly
# This should be replaced by virtual methods in a pure abstract object.
target_link_libraries( views PUBLIC
  payeeidentifier_iban_bic
  payeeidentifier_nationalAccount
  kmm_mymoney # needed to load payeeIdentifier
)

# we rely on some of the dialogs to be generated
add_dependencies(views dialogs newinvestmentwizard newloanwizard endingbalancedlg)

########### install files ###############

install(FILES ${libviews_a_HEADER}
  DESTINATION ${INCLUDE_INSTALL_DIR}/kmymoney COMPONENT Devel)
