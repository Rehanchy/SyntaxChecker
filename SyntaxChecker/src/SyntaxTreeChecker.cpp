#include "SyntaxTreeChecker.h"
#include <map>
#include <vector>

using namespace SyntaxTree;
typedef std::map<std::string, SyntaxTree::Type> IDmap;

void SyntaxTreeChecker::visit(Assembly& node) {
    for (auto def : node.global_defs) {
        def->accept(*this);
    }
}

void SyntaxTreeChecker::visit(FuncDef& node) {
    IDmap::iterator iter;
    iter = FuncTable.find(node.name);
    if(iter != FuncTable.end())
    {
        err.error(node.loc, "FuncDuplicated.");
        exit(int(ErrorType::FuncDuplicated));
    }
    else
        FuncTable[node.name] = node.ret_type;
    storeVec.push_back(Idtable);
    Idtable.clear();
    ParamListTable.clear(); // upper_eliminate
    node.param_list->accept(*this);
    /* get a map contains param list */
    /* combine it with IdTable */
    FuncParamTable[node.name] = ParamListTable; // store fparamlist
    /*IDmap::iterator paramiter;
    for(paramiter = Idtable.begin(); paramiter!=Idtable.end(); paramiter++)
        std::cout << paramiter->first;*/
    //ParamListTable.clear(); // into blkstmt
    node.body->accept(*this);
    ParamListTable.clear();
    Idtable = storeVec.back(); // restore Idtable
    storeVec.pop_back();
}

void SyntaxTreeChecker::visit(BinaryExpr& node) {
    node.lhs->accept(*this);
    bool lhs_int = this->Expr_int;
    SyntaxTree::Type lhs_type = this->Expr_type;
    node.rhs->accept(*this);
    bool rhs_int = this->Expr_int;
    SyntaxTree::Type rhs_type = this->Expr_type;
    if (node.op == SyntaxTree::BinOp::MODULO) {
        if (!lhs_int || !rhs_int) {
            err.error(node.loc, "Operands of modulo should be integers.");
            exit(int(ErrorType::Modulo));
        }
        if(lhs_type == SyntaxTree::Type::FLOAT || rhs_type == SyntaxTree::Type::FLOAT){
            err.error(node.loc, "Operands of modulo should be integers.");
            exit(int(ErrorType::Modulo));
        }
    }
    std::string out1 = (lhs_type == SyntaxTree::Type::INT) ? "int\n" : "float\n";
    std::string out2 = (rhs_type == SyntaxTree::Type::INT) ? "int\n" : "float\n";
    std::cout << out1 << out2;
    this->Expr_int = lhs_int & rhs_int;  // if both int then int
    this->Expr_type = (lhs_type == SyntaxTree::Type::FLOAT || rhs_type == SyntaxTree::Type::FLOAT) ? SyntaxTree::Type::FLOAT : SyntaxTree::Type::INT;
}

void SyntaxTreeChecker::visit(UnaryExpr& node) {
    node.rhs->accept(*this);   // maybe float a = 0.1
    bool rhs_int = this->Expr_int;
    this->Expr_int = rhs_int; 
}

void SyntaxTreeChecker::visit(LVal& node) {
    bool is_int;
    SyntaxTree::Type PassType; 
    std::cout << "visit LVAL\n";
    for (auto index : node.array_index) {
        index->accept(*this);
        is_int = this->Expr_int;
    }
    IDmap::iterator iter;
    iter = Idtable.find(node.name);
    if(iter != Idtable.end())
    {
        if(iter->second == SyntaxTree::Type::INT)
        { 
            is_int = true;
            PassType = SyntaxTree::Type::INT;
        }
        else
        {    
            is_int = false;
            PassType = SyntaxTree::Type::FLOAT;
        }
    }
    else
    {
        if(storeVec.empty())
        { 
            err.error(node.loc, "VarUnknown.");
            exit(int(ErrorType::VarUnknown));
        }
        std::vector<IDmap>::iterator mapiter;
        for(mapiter = storeVec.begin(); mapiter!=storeVec.end(); mapiter++)
        {
            IDmap::iterator iter_upper;
            IDmap storeTable = *mapiter;
            //IDmap::iterator paramiter;
            for(iter_upper = storeTable.begin(); iter_upper!=storeTable.end(); iter_upper++)
                std::cout << iter_upper->first;
            iter_upper = storeTable.find(node.name);
            if(iter_upper != storeTable.end()){
                std::cout<< "Id detected\n";   
                break;
            }
            else
            {
                std::cout<< "No Id detected\n";
                err.error(node.loc, "VarUnknown.");
                exit(int(ErrorType::VarUnknown));
            }
        }
    }
    this->Expr_int = is_int;
    this->Expr_type = PassType;
    this->Expr_ID = node.name;
    if(PassType == SyntaxTree::Type::FLOAT)
        std::cout << "float\n";
}

void SyntaxTreeChecker::visit(Literal& node) {
    this->Expr_int = (node.literal_type == SyntaxTree::Type::INT); // int then 1
    this->Expr_ID = (node.literal_type == SyntaxTree::Type::INT) ? "INT" : "FLOAT"; // -1. pass pass float
    this->Expr_type = node.literal_type;
}

void SyntaxTreeChecker::visit(ReturnStmt& node) {
    bool is_int;
    SyntaxTree::Type is_type;
    if (node.ret.get()) {
        node.ret->accept(*this);
        is_int = this->Expr_int;
        is_type = this->Expr_type;
    }
    this->Expr_int = is_int;
    this->Expr_type = is_type;
}

void SyntaxTreeChecker::visit(VarDef& node) {
    std::cout << "visit VarDef\n";
    if (node.is_inited) {
        node.initializers->accept(*this);
        //bool is_int = this->Expr_int;    
    }
    IDmap::iterator iter;
    iter = Idtable.find(node.name);
    if(iter != Idtable.end())
    {
        err.error(node.loc, "VarDuplicated.");
        exit(int(ErrorType::VarDuplicated));
    }
    else
        Idtable[node.name] = node.btype;
    if(node.btype == SyntaxTree::Type::INT)
        this->Expr_int = true;
    else
    { 
        this->Expr_int = false;
    }
    this->Expr_type = node.btype;
}

void SyntaxTreeChecker::visit(AssignStmt& node) {
    node.target->accept(*this);
    node.value->accept(*this);
    bool is_int = this->Expr_int;
    this->Expr_int = is_int;
}
void SyntaxTreeChecker::visit(FuncCallStmt& node) {
    bool is_int;
    size_t num = 0;
    SyntaxTree::Type Type_Check;
    std::string Exp_Name;
    std::cout<< "visit Funcall\n";
    IDmap LocalCallTable; // store paramlist passed by map
    IDmap::iterator iter;
    iter = FuncTable.find(node.name);
    if(iter != FuncTable.end())
    {
        LocalCallTable = FuncParamTable[node.name];
    }
    else
    {
        err.error(node.loc, "FuncUnknown.");
        exit(int(ErrorType::FuncUnknown));
    }
    int param_count = 0;
    for(auto exp:node.params)
    {
        std::cout<< "FuncCall in" << node.name << "\n";
        exp->accept(*this);
        IDmap::iterator iter;
        int i = 0;
        SyntaxTree::Type Temp_Type;
        for(iter = LocalCallTable.begin(); i <= param_count ; iter++)
        {    
            if(!(iter->first).empty())
            {
                Temp_Type = iter->second;
            }
            else
                break;
            i++;
        }
        param_count++;
        is_int = this->Expr_int;
        Exp_Name = this->Expr_ID; // this is where problem lies // Use an iter to read param one by one!
                                  // for each params, read one exp, iter++, iter->second == type.needed
        
        Type_Check = this->Expr_type;
        
        std::string out1 = (Type_Check == SyntaxTree::Type::INT) ? "int\n" : "float\n";
        std::string out2 = (Temp_Type == SyntaxTree::Type::INT) ? "int\n" : "float\n";
        std::cout << out1 << out2;
        if(Temp_Type != Type_Check)
        {
            err.error(node.loc, "typeerr,FuncParams.");
            exit(int(ErrorType::FuncParams));
        }
        IDmap::iterator paramiter;
        int number = 0;
        for(paramiter = LocalCallTable.begin(); paramiter!=LocalCallTable.end(); paramiter++)
            if(!(paramiter->first).empty())
                    number++;
        if(node.params.size()!=number)
        {
            //std::cout << node.params.size() << LocalCallTable.size();
            err.error(node.loc, "sizeerr,FuncParams.");
            exit(int(ErrorType::FuncParams));
        }
        if(num < node.params.size() - 1)
        {}
        num++;
    }
    std::cout<< "FuncCall out" << node.name << "\n";
    
    this->Expr_type = FuncTable[node.name];
    is_int = (FuncTable[node.name] == SyntaxTree::Type::INT) ? true: false;
    this->Expr_int = is_int;
    this->Expr_ID = node.name;
}
void SyntaxTreeChecker::visit(BlockStmt& node) {
    bool is_int;
    SyntaxTree::Type blk_type;
    std::cout<<"Visit Blkstmt\n";
    /*IDmap::iterator paramiter;
    for(paramiter = Idtable.begin(); paramiter!=Idtable.end(); paramiter++)
        std::cout << paramiter->first;*/  //ok
    storeVec.push_back(Idtable);
    Idtable.clear();
    Idtable.insert(ParamListTable.begin(), ParamListTable.end());
    ParamListTable.clear();
    for (auto stmt : node.body) {
        stmt->accept(*this);
        is_int = this->Expr_int;
        blk_type = this->Expr_type;
    }
    this->Expr_int = is_int;
    this->Expr_type = blk_type;
    Idtable = storeVec.back();
    storeVec.pop_back();
}
void SyntaxTreeChecker::visit(EmptyStmt& node) {}
void SyntaxTreeChecker::visit(SyntaxTree::ExprStmt& node) {
    bool is_int;
    SyntaxTree::Type ExprType;
    node.exp->accept(*this);
    is_int = this->Expr_int;
    ExprType = this->Expr_type;
    this->Expr_type = ExprType;
    this->Expr_int = is_int;
}
void SyntaxTreeChecker::visit(SyntaxTree::FuncParam& node) {
    bool is_int;
    if(node.param_type == SyntaxTree::Type::INT)
        is_int = true;
    else
        is_int = false;
    for(auto exp:node.array_index)
    {
        if (exp != nullptr)
            exp->accept(*this);
    }
    this->Expr_int = is_int;
    this->LocalType = node.param_type;
    this->LocalID = node.name;
}

void SyntaxTreeChecker::visit(SyntaxTree::FuncFParamList& node) {
    size_t num = 0;
    std::string passID;
    SyntaxTree::Type passType;
    //std::map<std::string, std::string> LocalIdTable;
    for(auto param:node.params)
    {
        param->accept(*this);
        passID = this->LocalID;
        passType = this->LocalType;
        IDmap::iterator iter;
        iter = ParamListTable.find(passID);
        if(iter != ParamListTable.end())
        {
            err.error(node.loc, "VarDuplicated.");
            exit(int(ErrorType::VarDuplicated));
        }
        else
            ParamListTable[passID] = passType;
        if(num < node.params.size() - 1)
        {
        }
        num++;
    }
}
void SyntaxTreeChecker::visit(SyntaxTree::BinaryCondExpr& node) {
    node.lhs->accept(*this);
    node.rhs->accept(*this);
    this->Expr_int = true;
}
void SyntaxTreeChecker::visit(SyntaxTree::UnaryCondExpr& node) {
    node.rhs->accept(*this);
    this->Expr_int = true;
}
void SyntaxTreeChecker::visit(SyntaxTree::IfStmt& node) {
    node.cond_exp->accept(*this);
    if(dynamic_cast<BlockStmt*>(node.if_statement.get())){
        node.if_statement->accept(*this);
    }
    else{
        node.if_statement->accept(*this);
    }
    if (node.else_statement != nullptr) {
        if(dynamic_cast<BlockStmt*>(node.else_statement.get())){
            node.else_statement->accept(*this);
        }else {
            node.else_statement->accept(*this);
        }
    }
}
void SyntaxTreeChecker::visit(SyntaxTree::WhileStmt& node) {
    node.cond_exp->accept(*this);
    if(dynamic_cast<BlockStmt*>(node.statement.get())){
        node.statement->accept(*this);
    }
    else{
        node.statement->accept(*this);
    }
}
void SyntaxTreeChecker::visit(SyntaxTree::BreakStmt& node) {

}
void SyntaxTreeChecker::visit(SyntaxTree::ContinueStmt& node) {

}

void SyntaxTreeChecker::visit(SyntaxTree::InitVal& node) {
    bool isValueInt;
    if (node.isExp) {
        node.expr->accept(*this);
        isValueInt = this->Expr_int;
    } 
    else {
        for (auto element : node.elementList) {
            element->accept(*this);
            isValueInt = this->Expr_int;
            //if(!isValueInt)
                //break;
        }
    }
    this->Expr_int = isValueInt;
    this->Expr_type = isValueInt ? SyntaxTree::Type::INT: SyntaxTree::Type::FLOAT;
}