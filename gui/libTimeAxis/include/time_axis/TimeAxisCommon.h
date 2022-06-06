#ifndef TIMEAXISCOMMON_H
#define TIMEAXISCOMMON_H

#include <vector>


namespace raco::time_axis{


#define INTERVAL_LENGTH_MIN 40
#define INTERVAL_LENGTH_DEFAULT 50
#define INTERVAL_LENGTH_MAX 80
#define INTERVAL_STEP 5
#define BASE_NUM 5
#define DEFAULT_BUTTON_WIDTH 20

//滚动条操作的选项
enum MOUSEACTION {
    MOUSE_SCROLL_DOWN = 0, //选中时间轴区域时，滚轮向下滚动
    MOUSE_SCROLL_UP = 1, //选中时间轴区域时，滚轮向上滚动
    MOUSE_LEFT_EXTEND = 2, //鼠标点击滚动条，向左扩展
    MOUSE_RIGHT_EXTEND = 3, //鼠标点击滚动条，向右扩展
    MOUSE_LEFT_MOVE = 4, //鼠标点击滚动条，向左移动
    MOUSE_RIGHT_MOVE = 5, //鼠标点击滚动条，向右移动
    MOUSE_NO_ACTION = 6
};

enum ANIMATIONACTION {
    ANIMATION_START = 0,
    ANIMATION_STOP = 1,
    ANIMATION_PAUSE = 2,
    ANIMATION_END = 3
};

}

#endif // TIMEAXISCOMMON_H

