# TMS320F28069M_LaunchPadXL
TMS320F28069M development board quick start 

- ## environment setup
- - Install Code Composer Studio Version: 11.2.0.00007 
- - Launch CCS -> Resource Explorer, click Software, C2000Ware 4.01.00, click install to get the documents offline.
- ## create default app, new CCS project.
- - Choose the DSP type number you are using. Select TI XDS100V2 which has been embedded on the board alreay. Select empty project (main.c)
- - Now you get your first program, you can try to debug / falsh to your board
- <img width="59" alt="image" src="https://user-images.githubusercontent.com/25374802/171591901-d0d6643f-6cc8-4d8e-8e72-5581129638f6.png">

- - The projram will run in the RAM for default. 
- ## add libray to your project
- - Right click your project name, properties, add these two folders bellow from C2000Ware you installed from CCS

- <img width="422" alt="image" src="https://user-images.githubusercontent.com/25374802/171592511-4568d459-c643-4c12-8b81-375b3a490ddf.png">

- - Add these basic core library source files: Right click project name, add files, choose these c files from C200Ware source folder, confirm to link it.

- <img width="336" alt="image" src="https://user-images.githubusercontent.com/25374802/171596210-1948164a-5858-4243-a217-313d1b950d1f.png">

- ## Run your first project

```
#include "DSP28x_Project.h"
void main(void)
{
    InitSysCtrl();
	while(1){
	    DELAY_US(1000000U);
	}
}
```
- - you can add a break point on line DELAY_US to catch the break every 1 second in debug mode.
