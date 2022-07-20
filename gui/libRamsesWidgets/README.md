# Ramses Composer使用说明

## 1.  切换坐标系功能

 1.  左上角的菜单栏中点击View-\>Project Settings

        ![](images/image1.png)

 2.  在Project Settings界面中找到"+Z up"选项

        ![](images/image2.png)

 3.  选中表示坐标系为+Z轴朝上，+Y朝屏幕内，预览窗口的坐标系图示切换成与设置坐标系一致

        ![](images/image3.png)

 4.  未选中表示坐标系为+Y轴朝上，+Z朝屏幕外，预览窗口的坐标系图示切换成与设置坐标系一致

        ![](images/image4.png)

 5.  点击"+Z up"选项切换坐标系后，Scene Graph中所有一级对象(对象类型包括Node，MeshNode, PerspectiveCamera,OrthographicCamera）的坐标都会自动切换为当前坐标系的坐标，切换规则如下：

        >假设切换前坐标为Translation（Tx, Ty, Tz）, Rotation(Rx, Ry, Rz),
        >切换后坐标为Translation（Tx', Ty', Tz'）, Rotation(Rx', Ry', Rz')
        >1.  +Z 朝上 -\> +Y朝上
            Translation：Tx' = Tx，Ty' = -Tz，Tz' = Ty
            Rotation: Rx' = Rx + 90
        >2.  +Y 朝上 -\> +Z朝上
            Translation：Tx' = Tx，Ty' = Tz，Tz' = -Ty
            Rotation: Rx' = Rx - 90

## 2.  导入新模型选则模型坐标系功能

 1.  在Scene Graph中创建Node节点，选中该对象，点击右键，点击"Import glTF Assets\..."

        ![](images/image5.png)

 2.  弹出"Import External Assets"弹窗，可以看到"Asseets Axes Direction"菜单，表示当前要导入的模型在建模时的坐标系方向，建模时的坐标系有可能与当前Ramses Composer中设置的坐标系方向不一致，因此需要知道建模时的坐标系，判断导入模型后是否需要对模型的顶点坐标做一次切换，最终导入模型后，顶点坐标为当前Ramses Composer中设置的坐标系下的坐标

        ![](images/image6.png)

## 3.  预览窗口显示网格线功能

  1.  预览窗口默认显示地面网格线，显示开关可以在"Project Settings"窗口中设置

  2.  Project Settings -\> Display Floor，选中表示显示网格线

        ![](images/image7.png)

  3.  Project Settings -\> Display Floor，不选中表示不显示网格线
 
        ![](images/image8.png)

## 4.  视角切换功能

预览窗口中提供Roam和Preview两种视角，两种视角下的camera参数可分别调节（目前Roam视角的camera参数调节功能还未完成），两种视角可以互相切换

   1.  Roam表示漫游视角，仅用于在Ramses Composer中显示，与最终在车机中的显示效果无关，在此视角下，可响应鼠标滑轮的放大缩小事件来改变模型的位置和大小

   2.  Preview表示预览视角，是最终在车机中的显示效果，在此视角下，不响应鼠标滑轮的放大缩小事件，模型显示位置和大小仅能通过调整camera参数来改变

       ![](images/image9.png)
