%module kmymoney_python

%{
#include "transactionbuilder.h"
%}

%include "std_string.i"

namespace KMyMoney {
namespace Python {
class TransactionBuilder
{
public:
    TransactionBuilder();

    void setMemo(const std::string& memo);
    std::string memo() const;

    void setCommodity(const std::string& commodityId);
    std::string commodity() const;

    void setPostDate(int year, int month, int day);
    void setEntryDate(int year, int month, int day);

    void addSplit(const std::string& accountId, const std::string& value, const std::string& memo = std::string());

    unsigned int splitCount() const;
    bool isBalanced() const;
};
}
}
