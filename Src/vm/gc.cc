/**
 * Def 对象分配及垃圾回收
 */

#include <string>
 
#include "gc.h"

using namespace std;

using namespace def::object;
using namespace def::node;
using namespace def::gc;



/**
 * 构造初始化
 */
Gc::Gc(){
	// 初始化小对象
	prep_none = new ObjectNone();
	prep_true = new ObjectBool(true);
	prep_false = new ObjectBool(false);
	// 初始化小整数列表
	for(int i=0; i<270; i++){
		prep_ints[i] = new ObjectInt(i-10);
	}
	// 初始化小字符串列表
	// TODO::
}


/**
 * 创建 Int 对象
 */
ObjectBool* Gc::AllotBool(bool val)
{
	return val ? prep_true : prep_false;
}


/**
 * 创建 Int 对象
 */
ObjectInt* Gc::AllotInt(long val)
{
	//cout<<"int = "<<val<<endl;
	if(val<260&&val>=-10){
		// 小整数池 
		//cout<<"get from mini int poll"<<endl;
		return prep_ints[val+10];
	}
	if(0==free_int.size()){
		// cout<<"new ObjectInt"<<endl;
		return new ObjectInt(val);
	}
	// 取自 int 空闲内存池
	// cout<<"get from free int poll"<<endl;
	ObjectInt* pi = (ObjectInt*)free_int.back();
	free_int.pop_back();
	pi->value = val; // 改值
	return pi; 
}


/**
 * 从语法节点分配新的对象
 */
DefObject* Gc::Allot(Node* n)
{

#define T NodeType

	T t = n->type;

	if(t==T::None){ // none

		return prep_none;

	}else if(t==T::Bool){ // bool

		return AllotBool(n->GetBool());

	}else if(t==T::Int){ // int

		return AllotInt(n->GetInt());

	}else if(t==T::String){ // string

	}

#undef T

}


#define T ObjectType
#define IS_CONTAINER_OBJ obj->type==T::List||obj->type==T::Dict
// 判断是否为小整数
#define IF_MINI_INT_OBJ \
		ObjectInt* obj_int = (ObjectInt*)obj; \
		long val = obj_int->value; \
		if(val<260&&val>=-10)



/**
 * 引用现有的对象
 * 引用计数 +1
 */
DefObject* Gc::Quote(DefObject* obj)
{
	//cout<<"Gc::Quote"<<endl;
	T t = obj->type;
	if(t==T::None||t==T::Bool){
		return obj; // 小对象
	}
	if(t==T::Int){
		IF_MINI_INT_OBJ{
			return obj; // 小整数
		}
	}
	// 引用计数 +1
	obj->refcnt += 1;
	//cout<<"quote refcnt = "<<obj->refcnt<<endl;
	return obj;
}


/**
 * 释放对象
 * 引用计数 -1
 * 当引用计数变为0时回收对象，并返回 true 否则返回 false
 * 递归释放容器对象
 */
bool Gc::Free(DefObject* obj)
{
	//cout<<"Gc::Free"<<endl;
	T t = obj->type;
	size_t r = obj->refcnt;
	// 递归释放容器对象
	if(IS_CONTAINER_OBJ){
	   	// TODO:: 
	}
	if(r<=1){ // 引用归零 回收对象
		return Recycle(obj);
	}
	// 更新引用计数
	//cout<<"free refcnt = "<<(r-1)<<endl;
	obj->refcnt = r-1;
	return false;
}


/**
 * 回收对象
 * 递归容器对象 判断是否回收
 */
bool Gc::Recycle(DefObject* obj)
{
	// cout<<"Gc::Recycle"<<endl;
	T t = obj->type;
	if(t==T::None||t==T::Bool){
		return true; // 小对象不需要 del
	}
	if(t==T::Int){
		IF_MINI_INT_OBJ{ // 小整数 不需要 del
			//cout<<"IF_MINI_INT_OBJ"<<endl;
			return true;
		}
		if(free_int.size()<10){ // 限制空闲列表大小
			obj->refcnt = 0; //引用归零
			free_int.push_back(obj); //保存至空闲内存
			// cout<<"free_int.push  size="<<free_int.size()<<endl;
			return true;
		}
	}
	// cout<<"delete obj"<<endl;
	delete obj; // delete 对象指针 
	return true;
}


#undef T
#undef IS_CONTAINER_OBJ