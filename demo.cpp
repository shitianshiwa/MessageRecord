#include <cqcppsdk/cqcppsdk.h>
#include <sstream>
#include <stack>
#include <time.h>
#include <map>

using namespace cq;
using namespace std;

auto add_group_map(const cq::GroupMessageEvent &e);                      //注册该群map
auto get_sender_nickname(const cq::GroupMessageEvent &e)->string;         //获取发送人昵称
auto get_sent_time (const cq::GroupMessageEvent &e) ->string;              //获取发送时间
auto message_record (const cq::GroupMessageEvent &e);                       //读取群map消息栈，组装消息体并压入消息栈
auto add_message(const cq::GroupMessageEvent &e);                            //将消息栈更新到该群的map
auto message_reproduce(const cq::GroupMessageEvent &e) ->string;              //从该群map读取消息栈，并更新群map的消息栈

typedef struct sent_info {
//  std::string qq;
    std::string sent_time;
    std::string sender;
    std::string message;
}SENT_INFO;

int MSGMAX;                      //消息栈最大容量
int64_t adminer = NULL;          //插件管理员
std::map <int64_t, std::stack <SENT_INFO>> group_map;


auto add_group_map(const cq::GroupMessageEvent &e) {
    std::stack <SENT_INFO> message_stack;
    group_map.insert(pair<int64_t, std::stack <SENT_INFO>>(e.group_id,message_stack));
}

auto remove_group_map(const cq::GroupMessageEvent &e) -> string {
    if(group_map.erase(e.group_id)) {
        return "随风潜入夜,润物细无声";
    } else {
        return "developed by QiJieH";
    }
}


auto get_sender_nickname(const cq::GroupMessageEvent &e) -> string {
    auto group_member_info = cq::get_group_member_info(e.group_id, e.user_id);
    if (!group_member_info.card.empty()){
        return group_member_info.card;                 //如果群昵称不为空，返回群昵称
    } else {
        return group_member_info.nickname;             //没有设置群昵称，返回QQ昵称
    }
}
auto get_sent_time (const cq::GroupMessageEvent &e) ->string {
    char sent_time[32];
    time_t t = time(NULL);
    strftime(sent_time,sizeof(sent_time),"%H:%M",localtime(&t));
    return sent_time;
}
auto message_record (const cq::GroupMessageEvent &e) {
    std::stack <SENT_INFO> message_stack =  group_map[e.group_id];
    //如果消息栈达到最大值，自动清空所有栈内元素
    if(message_stack.size()==MSGMAX) {
        while (!message_stack.empty()) {
            message_stack.pop();
        }
    }

    sent_info SI_t = {"developer","QiJieH","Powered by coolq"};       //初始化局部变量
    SI_t.message = e.message;
    SI_t.sender = get_sender_nickname(e);
    SI_t.sent_time = get_sent_time(e);
//  SI_t.qq = e.user_id;
    message_stack.push(SI_t);
    return message_stack;
}

auto add_message(const cq::GroupMessageEvent &e){
    if(group_map.count(e.group_id)==false){return;}
    //if(e.user_id == adminer){return};                     //不记录插件管理员消息
    group_map[e.group_id] = message_record(e);
}

auto message_reproduce(const cq::GroupMessageEvent &e , int callbacks /*,std::string qqnumber */) ->string{
    if(group_map.count(e.group_id)==false){return "该群未开启消息回放";}
    std::stack <SENT_INFO> message_stack = group_map[e.group_id];
    if(message_stack.empty()){
        return "无记录";
    }

    while (callbacks>1)
    {
        message_stack.pop();
        callbacks--;
    }
    
    auto SI_t = message_stack.top();
    message_stack.pop();
    group_map[e.group_id] = message_stack;
    return SI_t.sender+"在"+SI_t.sent_time+"发送：\r\n"+SI_t.message;
}

int string_to_int(std::string str) {
    int c_int;
    stringstream stream(str);
    stream>>c_int;
    return c_int;
}


CQ_INIT {
    MSGMAX = 32;
    //管理员注册及初始化设定
    cq::on_private_message([](const auto &event) {
        if(event.message=="#消息回放管理员注册"){
           
            if(adminer==NULL) {
                adminer = event.user_id;
                send_message(event.target, "已注册当前QQ为管理员");
                send_message(event.target, "您可在QQ群中发送#开启消息回放或#关闭消息回放来管理是否在该群开启消息回放");
            }
            if(event.user_id!=adminer && adminer!=NULL){
                send_message(event.target, "您没有权限！");
            }
        }
        if(event.user_id == adminer && event.message == "#设置消息栈容量大小") {
            send_message(event.target, "发送相应指令设置，仅支持预设值：#32 #64 #128");
        }
        if(event.user_id == adminer) {
            if(event.message == "#32") MSGMAX = 32;
            if(event.message == "#64") MSGMAX = 64;
            if(event.message == "#128") MSGMAX = 128;
            // 非用户接口
            if(event.message == "#256") MSGMAX = 256;
            if(event.message == "#512") MSGMAX = 512;
        }
    });
    //触发命令
    on_group_message([](const auto &event){

        if(event.user_id == adminer){
            if(event.message == "#开启消息回放"){
                add_group_map(event);
                send_group_message(event.group_id, "心事浩茫连广宇,于无声处听惊雷");
                return;
            }
            if(event.message == "#消息回放"){
                send_group_message(event.group_id, message_reproduce(event,1));
                return;
            }
            //精准消息回放
            if(event.message.find("#消息回放*")==0) {
                std::string c_number;
                c_number.assign(event.message, 14, event.message.length()-14);
                send_group_message(event.group_id, message_reproduce(event,string_to_int(c_number)));
                return;

            }
            //逆序消息回放
            if(event.message.find("#消息回放x")==0){
                std::string c_number;
                c_number.assign(event.message, 14, event.message.length()-14);
                int c_int = string_to_int(c_number);
                while(c_int){
                    send_group_message(event.group_id, message_reproduce(event,1));
                    c_int--;
                }
                return;
            }
            if(event.message == "#关闭消息回放"){
                send_group_message(event.group_id, remove_group_map(event));
                return;
            }
            if(event.message == "#developer"){
                send_group_message(event.group_id, "developed by QiJieH : ");
                return;
            }
        } else {
            add_message(event);
        }
    });
}