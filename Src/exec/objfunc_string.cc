/**
 * 对象函数 string
 */
DO* Exec::ObjfuncString(string base, string func, Node* para)
{
    // cout<<"-Exec::ObjfuncString-"<<endl;
    LOCALIZE_gc
    NodeGroup* argv = (NodeGroup*) para;
    size_t len = argv->ChildSize();

    // 当前文件所在文件
    string path = Path::getDir(_envir._file);
    // cout<<"Path::getDir(_envir._file) = "<<path<<endl;
    DefObject* o1 = NULL;
    DefObject* o2 = NULL;
    if(len>0){
        o1 = Evaluat( para->Child(0) );
    }
    if(len>1){
        o2 = Evaluat( para->Child(1) );
    }

    // 取单个字符
    if(func=="at"){
        if(o1 && o1->type==OT::Int){
            int idx = ((ObjectInt*)o1)->value - 1; // 索引从1开始
            if( idx>=0 && idx < base.size() ){
                return _gc->AllotString( base.substr(idx,1) );
            }
        }
        return ObjNone();


    // 截取字符串
    }else if(func=="substr"){

    // 替换字符串
    }else if(func=="replace"){
        string nstr = base;
        if( o1 && o2 && o1->type==OT::String && o2->type==OT::String ){
            Str::replace_all(nstr, ((ObjectString*)o1)->value, ((ObjectString*)o2)->value);
        }
        return _gc->AllotString( nstr );
    }

}

