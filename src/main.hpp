/******************************************************INCLUDES************************************************************/
#include "main.h"
#include "okapi/api.hpp"
#include "okapi/api/chassis/model/xDriveModel.hpp"
#include "pros/misc.h"
#include <stdio.h>
#include <errno.h>
#include "display/lv_objx/lv_label.h"
#include "display/lv_themes/lv_theme_night.h"
#include "display/lv_themes/lv_theme_night.c"
#include "D:\Documents\ProsCode\GameCode\src\autonRun1.hpp"

/******************************************************GLOBALS************************************************************/
int LeftDriveFrontSpeed;	            //first motor
int LeftDriveBackSpeed;          //third motor
int LeftIntakeSpeed;           //second motor
int RightDriveFrontSpeed;        //fourth motor
int RightDriveBackSpeed;		      //sixth motor
int RightIntakeSpeed;         //fifth motor
int DropperSpeed;               //seventh motor
int FourBarSpeed;
int timeOld;
int timeNew;
int deltaTime;

// hi

// ADIUltrasonic sonar('G','H');
// ADIButton button('A');
// ADIUltrasonic sensor ('A', 'C');
// int cataSonarSensor = sonar.get();

Controller controller;               // Joystick to read analog values for tank or arcade control.
// Master controller by
extern pros::Vision vision;

ControllerButton rightUp = controller[ControllerDigital::R1];                   //lift up right arm
ControllerButton rightDown = controller[ControllerDigital::R2];                 //put down right arm
ControllerButton leftUp = controller[ControllerDigital::L1];                    //lift up left arm
ControllerButton leftDown = controller[ControllerDigital::L2];                  //put down left arm

ControllerButton btnDown = controller[ControllerDigital::down];
ControllerButton btnUp = controller[ControllerDigital::up];                     //brakes
ControllerButton btnRight = controller[ControllerDigital::right];               //stop intake
ControllerButton btnLeft = controller[ControllerDigital::left];                 //reverse drive
ControllerButton btnA = controller[ControllerDigital::A];
ControllerButton btnB = controller[ControllerDigital::B];
ControllerButton btnX = controller[ControllerDigital::X];
ControllerButton btnY = controller[ControllerDigital::Y];

float leftY = controller.getAnalog(ControllerAnalog::leftY);
float leftX = controller.getAnalog(ControllerAnalog::leftX);
float rightY = controller.getAnalog(ControllerAnalog::rightY);
float rightX = controller.getAnalog(ControllerAnalog::rightX);


MotorGroup left = {LeftDriveFront, LeftDriveBack};
MotorGroup right = {RightDriveFront, RightDriveBack};

auto drive =  ChassisControllerFactory::create(
	left, right,
	AbstractMotor::gearset::green,
	{2.75_in, 10.5_in}
);

bool bringDropperBack = false;
bool bringLiftBack = false;
bool isRecording = false;

float dropperMax = 1400;
float dropperError;
int dropperTargetSpeed;
int d_N = 90;
float d_d = 5;
int d_m = 10;
float e = 2.71828;

float intakeMultiplier = 45;
static void text(int l, std::string message);


/*****************************************************vision**************************************************************/
extern pros::vision_signature red;
extern pros::vision_signature blue;
extern pros::vision_signature green;
// extern pros::vision_color_code_t redFlagColorCode;
// extern pros::vision_color_code_t blueFlagColorCode;
// static pros::vision_color_code_t theFlagIAmShooting = redFlagColorCode;



/*****************************************************LV stuff**************************************************************/
const char * motorMapLayout[] = {"\223FL", "\242", "\223FR", "\242", "\n",
"\223BL", "\242", "\223BR", "\242", "\n",
"\223Left In", "\242", "\223Right In", "\242", "\n",
"\223Dropper", "\242", "\2234Bar", "\242", ""};
static lv_obj_t * scr;
static lv_style_t abar_indic;
lv_obj_t * textContainter;


lv_obj_t * lines[10];

void text(int l, std::string message){
	const char* c = message.c_str();
	lv_label_set_text(lines[l], c);
}
extern lv_theme_t * lv_theme_night_init(uint16_t hue, lv_font_t *font);


std::string motorNameString;
static lv_res_t btnm_action(lv_obj_t * btnm, const char * txt){
	if((std::string)(txt) == "FL"){
		DisplayMotor = {LeftDriveFront};
	}else if((std::string)(txt) == "BL"){
		DisplayMotor = {LeftDriveBack};
	}else if((std::string)(txt) == "FR"){
		DisplayMotor = {RightDriveFront};
	}else if((std::string)(txt) == "BR"){
		DisplayMotor = {RightDriveBack};
	}else if((std::string)(txt) == "Left In"){
		DisplayMotor = {LeftIntake};
	}else if((std::string)(txt) == "Right In"){
		DisplayMotor = {RightIntake};
	}else if((std::string)(txt) == "Dropper"){
		DisplayMotor = {Dropper};
	}else if((std::string)(txt) == "4Bar"){
		DisplayMotor = {FourBar};
	}
	motorNameString = (std::string)txt;
	return LV_RES_OK;
}
int batteryCharge;


/*****************************************************OPCONTROL***********************************************************/
//285 = 90 degy

lv_obj_t * hubTab;
lv_obj_t * textTab;
lv_obj_t * visionTab;


void intake(int speed){
	LeftIntake.moveVelocity(speed);
	RightIntake.moveVelocity(speed);
}
void intakeDist(int dist, int speed){
	LeftIntake.moveAbsolute(dist, speed);
	RightIntake.moveAbsolute(dist, speed);
}
/**
 * A callback function for LLEMU's center button.
 *
 * When this callback is fired, it will toggle line 2 of the LCD text between
 * "I was pressed!" and nothing.
 */

//array of autons
std::string arr[] = {"red vertical (0)", "red horizontal (1)", "blue vertical (2)", "blue horizontal (3)", "Preload(4)", "blue skills (5)"};
int autNum = 0; //selected auton
void on_center_button() {
		pros::lcd::set_text(0, arr[autNum]); //does nothing

}

void on_right_button(){ //increment autNum to next auton
		 autNum = autNum + 1;
		 autNum = autNum%6;
		 pros::lcd::set_text(0, arr[autNum]);
}

void on_left_button(){ //decrement autNum and check for negative
	autNum = autNum - 1;
	if(autNum < 0){
		autNum = autNum + 6;
	}
	autNum = autNum%6;
	pros::lcd::set_text(0, arr[autNum]);
}

/**
 * Runs initialization code. This occurs as soon as the program is started.
 *
 * All other competition modes are blocked by initialize; it is recommended
 * to keep execution time for this mode under a few seconds.
 */
void initialize() {
	pros::lcd::initialize();
	pros::lcd::register_btn0_cb(on_left_button);
	pros::lcd::register_btn1_cb(on_center_button);
	pros::lcd::register_btn2_cb(on_right_button);

}
/**
 * Runs while the robot is in the disabled state of Field Management System or
 * the VEX Competition Switch, following either autonomous or opcontrol. When
 * the robot is enabled, this task will exit.
 */
void disabled() {

}

/**
 * Runs after initialize(), and before autonomous when connected to the Field
 * Management System or the VEX Competition Switch. This is intended for
 * competition-specific initialization routines, such as an autonomous selector
 * on the LCD.
 *
 * This task will exit when the robot is enabled and autonomous or opcontrol
 * starts.
 */
void competition_initialize() {
	while(true){
		if(btnLeft.isPressed()){
		 autNum++;
		 autNum = autNum%4;
	 }else if(btnRight.isPressed()){
		 autNum--;
		 if(autNum < 0){
			 autNum += 4;
		 }
		 autNum = autNum%4;
	 }
	 pros::c::lcd_set_text(0,  arr[autNum].c_str());
	 pros::delay(50);
 	}
}

/**
 * Runs the user autonomous code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the autonomous
 * mode. Alternatively, this function may be called in initialize or opcontrol
 * for non-competition testing purposes.
 *
 * If the robot is disabled or communications is lost, the autonomous task
 * will be stopped. Re-enabling the robot will restart the task, not re-start it
 * from where it left off.
 */
void moveDrive(int dist, int pow){
	left.moveRelative(dist, pow);
	right.moveRelative(dist, pow);
}

void blueVertical(){
	FourBar.moveAbsolute(-700, 200);
	Dropper.moveAbsolute(800, 200);
	pros::delay(1950);

	//move 4-bar to height of 2nd cube
	FourBar.moveAbsolute(-120, 200);

	pros::delay(1300);


	intake(120); // start intake
	//set drive max speed
	drive.setMaxVelocity(120);
	//move drive forward 790 encoder units
	drive.moveDistance(790);
	pros::delay(100);
	//slow down drive
	drive.setMaxVelocity(80);
	//go forward 420 encoder units
	drive.moveDistance(420);
	/*by now the 2nd cube is in the intake,
	the 3rd and 4th cube are in the tray, the bottom
	cube was pushed 1/4th of tile forwards*/


	//lower the 4bar and the dropper
	Dropper.moveAbsolute(0, 200);
	FourBar.moveAbsolute(0, 200);
	//back up
	drive.moveDistance(-100);
	//go forward and intake bottom cube
	drive.moveDistance(200);
	//intake for a while
	pros::delay(500);
	//stop intake
	intake(0);
	//turn and face the scoring zone
	drive.turnAngle(-410);
}//blueVertical

void bluePreload(){
	drive.moveDistance(600);
	drive.moveDistance(-400);
}
void blueHorizontal(){
	intakeDist(-220, 200);
	pros::delay(1000);

	FourBar.moveAbsolute(-600, 200);
	pros::delay(1200);

	FourBar.moveAbsolute(-20, 200);
	intake(200);
	drive.setMaxVelocity(100);
	drive.moveDistance(1240);
	pros::delay(500);
	intake(0);
	drive.setMaxVelocity(90);
	drive.moveDistance(-740);
	// drive.turnAngle(-420);
	// 	intake(-15);
	// drive.moveDistance(285);
	// moveDrive(300,80);
	// Dropper.moveAbsolute(2200, 200);
	// pros::delay(2300);
	// Dropper.moveAbsolute(2800, 100);
	// pros::delay(1000);
	// intake(-45);
	// drive.setMaxVelocity(40);
	// drive.moveDistance(-500);
}
void redHorizontal(){
	intakeDist(-250, 200);
	pros::delay(300);

	FourBar.moveAbsolute(-600, 200);
	pros::delay(1200);

	FourBar.moveAbsolute(-20, 200);
	intake(200);
	drive.setMaxVelocity(80);
	drive.moveDistance(1100);
	pros::delay(500);
	drive.setMaxVelocity(90);
	drive.moveDistance(-800);
	intake(0);
	// intake(-20);
	drive.turnAngle(310);
	drive.moveDistance(275);
	moveDrive(200,10);
	Dropper.moveAbsolute(2950, 200);

	// pros::delay(3000);
	// intake(-30);
	// drive.setMaxVelocity(30);
	// drive.moveDistance(-500);
}

void blueProgrammingSkills(){
	FourBar.moveAbsolute(-650, 200);
	pros::delay(1000);
	FourBar.moveAbsolute(10, 200);
	Dropper.moveAbsolute(-200, 200);
	pros::delay(1000);
	intake(200);
	drive.setMaxVelocity(75);
	drive.moveDistance(1300);
	drive.turnAngle(50);
	intake(-50);
	pros::delay(200);
	intake(50);
	pros::delay(200);
	drive.moveDistance(150);
	drive.moveDistance(-150);
	drive.turnAngle(-50);
	pros::delay(500);
	drive.setMaxVelocity(90);
	drive.moveDistance(-750);
	intake(-18);
	drive.turnAngle(-410);
	drive.moveDistance(285);
	intake(0);
	moveDrive(300,40);
	Dropper.moveAbsolute(3000, 200);
	pros::delay(5000);
	drive.setMaxVelocity(80);
	drive.moveDistance(-500);
	intake(15);
	drive.turnAngle(410);
	drive.forward(-120);
	pros::delay(2500);
	drive.moveDistance(474);
	drive.turnAngle(300);
	Dropper.moveAbsolute(0, 200);
	pros::delay(4000);
	intake(200);
	drive.moveDistance(840);
	intake(0);
	FourBar.moveAbsolute(-803, 200);
	drive.moveDistance(-200);
	pros::delay(1000);
	drive.moveDistance(50);
	intake(-100);
	pros::delay(1000);
	intake(0);

}

void blueHorizontal2(){
	drive.setBrakeMode(AbstractMotor::brakeMode::coast);
	FourBar.setBrakeMode(AbstractMotor::brakeMode::hold);
	drive.setMaxVelocity(90);
	intake(200);

	drive.moveDistance(1070);
	pros::delay(300);

	intake(0);

	drive.setBrakeMode(AbstractMotor::brakeMode::brake);
	drive.setMaxVelocity(65);
	drive.turnAngle(-175);
	drive.setBrakeMode(AbstractMotor::brakeMode::coast);


	drive.setMaxVelocity(80);
	intake(200);
	drive.moveDistance(500);
	pros::delay(300);

	intake(-10);

	left.tarePosition();
	right.tarePosition();

	Dropper.moveAbsolute(700, 200);
	while(left.getPosition()>-370){
		if(left.getPosition()>-100){
			drive.arcade(-.7,0);
		}else{
			drive.arcade(-.6,-.75);
		}
		pros::delay(100);
	}
	left.tarePosition();
	right.tarePosition();

	drive.arcade(0,0);
	pros::delay(2000);
	intake(200);
	Dropper.moveAbsolute(0, 200);
	while(right.getPosition()<800){
		if(right.getPosition()<100){
			drive.arcade(.6,0);
		}else{
			drive.arcade(.5,-.3);

		}
		pros::delay(100);
	}
	drive.arcade(0,0);
	intake(0);

}
void autonomous() {
 // run1();
	// if(autNum == 0){
	// 	redVertical();
	// }else if(autNum == 1){
	// 	redHorizontal();
	// }else if(autNum == 2){
	// 	blueVertical();
	// }else if(autNum == 3){
	// 	blueHorizontal();
	// }else if (autNum == 4){
	// 	redPreload();
	// }else{
	// 	blueProgrammingSkills();
	// }
	// redHorizontal();

	// drive.setMaxVelocity(100);
	// drive.moveDistance(-400);
	// drive.moveDistance(400);
	//
	// intake(-200);
	// pros::delay(1000);
	// intake(0);



	// drive.moveDistance(-400);
	// drive.moveDistance(100);
	// drive.turnAngle(60);
	// drive.moveDistance(400);
	// intake(-90);
	// FourBar.moveAbsolute(-780, 200);
	// pros::delay(1500);
	// pros::delay(1000);
	// intake(0);
	// intake(90);
	//
	// FourBar.moveAbsolute(-20, 200);
	// Dropper.moveAbsolute(2200, 200);
	// pros::delay(3700);
	//
	// Dropper.moveAbsolute(0, 200);
	// pros::delay(3000);


}

/**
 * Runs the operator control code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the operator
 * control mode.
 *
 * If no competition control is connected, this function will run immediately
 * following initialize().
 *
 * If the robot is disabled or communications is lost, the
 * operator control task will be stopped. Re-enabling the robot will restart the
 * task, not resume it from where it left off.
 */


 void opcontrol() {

   // set up the screen
   lv_theme_set_current(lv_theme_night_init(0, NULL));
   scr = lv_page_create(NULL, NULL);
   lv_obj_set_size(scr, 480, 272);
   lv_scr_load(scr);

   lv_obj_t * tabview = lv_tabview_create(lv_scr_act(), NULL);
   hubTab = lv_tabview_add_tab(tabview, "Hub");
   textTab = lv_tabview_add_tab(tabview, "Text");
   visionTab = lv_tabview_add_tab(tabview, "Vision");


   //  lv_obj_set_size(tabview, 10, 10);


   lv_obj_t * motorButtons = lv_btnm_create(hubTab, NULL);
   lv_btnm_set_map(motorButtons, motorMapLayout);
   lv_obj_set_pos(motorButtons, 0, 0);
   lv_obj_set_size(motorButtons, 170, 130);
   lv_btnm_set_action(motorButtons, btnm_action);
   lv_obj_t * motorContainer = lv_cont_create(hubTab, NULL);
   lv_obj_set_pos(motorContainer, 200, 55);
   lv_obj_set_size(motorContainer, 200, 100);
   lv_obj_t * motorName = lv_label_create(motorContainer, NULL);
   lv_obj_set_pos(motorName, 0, 5);
   lv_obj_t * motorTemp = lv_label_create(motorContainer, NULL);
   lv_obj_align(motorTemp, motorName, LV_ALIGN_OUT_BOTTOM_MID, 5, 3);
   lv_obj_t * motorPosition = lv_label_create(motorContainer, NULL);
   lv_obj_align(motorPosition, motorTemp, LV_ALIGN_OUT_BOTTOM_MID, 5, 3);

   lv_obj_t * batteryBar = lv_bar_create(hubTab, NULL);
   lv_obj_set_size(batteryBar, 220, 20);
   lv_obj_set_pos(batteryBar, 185, -20);
   lv_bar_set_range(batteryBar, 0, 100);
   lv_obj_align(batteryBar, motorButtons, LV_ALIGN_OUT_TOP_RIGHT, 50, 0);
   lv_obj_t * batteryLabel = lv_label_create(hubTab, NULL);
   lv_obj_align(batteryLabel, batteryBar, LV_ALIGN_IN_RIGHT_MID, 50, 0);
   lv_label_set_text(batteryLabel, ("" + std::to_string(pros::battery::get_capacity())+ "%").c_str() );
   lv_obj_set_size(batteryLabel, 240, 15);
   lv_obj_t * TLbar = lv_bar_create(motorButtons, NULL);
   lv_bar_set_range(TLbar, 25, 60);
   lv_obj_set_size(TLbar, 10, 25);
   lv_obj_set_pos(TLbar, 60, 3);
   lv_obj_t * TRbar = lv_bar_create(motorButtons, TLbar);
   lv_obj_align(TRbar, TLbar, LV_ALIGN_OUT_RIGHT_MID, 76, 0);
   lv_obj_t * CLbar = lv_bar_create(motorButtons, TLbar);
   lv_obj_align(CLbar, TLbar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
   lv_obj_t * BLbar = lv_bar_create(motorButtons, TLbar);
   lv_obj_align(BLbar, CLbar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
   lv_obj_t * Catabar = lv_bar_create(motorButtons, TLbar);

   lv_obj_align(Catabar, BLbar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
   lv_obj_t * CRbar = lv_bar_create(motorButtons, TLbar);
   lv_obj_align(CRbar, TRbar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
   lv_obj_t * BRbar = lv_bar_create(motorButtons, TLbar);
   lv_obj_align(BRbar, CRbar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
   lv_obj_t * Intakebar = lv_bar_create(motorButtons, TLbar);
   lv_obj_align(Intakebar, BRbar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);


   textContainter = lv_cont_create(textTab, NULL);
   lv_obj_align(textContainter, textTab, LV_ALIGN_IN_TOP_LEFT, 0, 0);
   lv_obj_set_size(textContainter, 480, 240);

   for(int i = 0; i < 10; i++){
     lines[i] = lv_label_create(textContainter, NULL);
     lv_obj_set_x(lines[i], 15);
     lv_obj_set_y(lines[i], 20*i);
     lv_label_set_text(lines[i], "");
   }
	 //open file for auton recorder
	 if(isRecording){
		 FILE* autonWriter = fopen("/usd/run1.txt", "w");
		 fprintf(autonWriter, "");
		 fclose(autonWriter);
 	 }
   //initialize brake modes
   drive.setBrakeMode(AbstractMotor::brakeMode(0));
	 FourBar.setBrakeMode(AbstractMotor::brakeMode::hold);
	 LeftIntake.setBrakeMode(AbstractMotor::brakeMode::hold);
	 RightIntake.setBrakeMode(AbstractMotor::brakeMode::hold);
	 Dropper.setBrakeMode(AbstractMotor::brakeMode::hold);
	 int shakeIntake = 1;


   while(true){

     //lvgl stuff///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
     batteryCharge = (int)(pros::battery::get_capacity());
     lv_label_set_text(batteryLabel, ("" + std::to_string(pros::battery::get_capacity())+ "%").c_str() );
     lv_bar_set_value(batteryBar, batteryCharge);
     lv_bar_set_value(TLbar, LeftDriveBack.getTemperature());
     lv_bar_set_value(TRbar, RightDriveBack.getTemperature());
     lv_bar_set_value(CLbar, LeftIntake.getTemperature());
     lv_bar_set_value(CRbar, RightIntake.getTemperature());
     lv_bar_set_value(BLbar, LeftDriveFront.getTemperature());
     lv_bar_set_value(BRbar, LeftDriveBack.getTemperature());
     lv_bar_set_value(Catabar, FourBar.getTemperature());
     lv_bar_set_value(Intakebar, Dropper.getTemperature());

     lv_label_set_text(motorName, ("Motor --- " + motorNameString).c_str());
     lv_label_set_text(motorTemp, ("Temp : " + std::to_string(DisplayMotor.getTemperature())).c_str());
     lv_label_set_text(motorPosition, ("Pos : " + std::to_string(DisplayMotor.getPosition())).c_str());



     //drive code///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
     leftY = controller.getAnalog(ControllerAnalog::leftY);
     leftX = controller.getAnalog(ControllerAnalog::leftX);
     rightY = controller.getAnalog(ControllerAnalog::rightY);
     rightX = controller.getAnalog(ControllerAnalog::rightX);





		 if(rightUp.isPressed() && rightDown.isPressed()){

		 }else if(!btnA.isPressed()){
			 if(rightUp.isPressed()){					//4-Bar up
			 	FourBar.moveVelocity(-200);
			}else if(rightDown.isPressed()){
			 	FourBar.moveVelocity(200);		//4-Bar down
			 }else{														//4-Bar off
			 	FourBar.moveVelocity(0);
			 }
	 	}else{

		}

		if(leftUp.isPressed() && leftDown.isPressed()){
				if(FourBar.getPosition() < -400){
					LeftIntake.moveVelocity(-200);
					RightIntake.moveVelocity(-200);
				}else{
					LeftIntake.moveVelocity(200);
					RightIntake.moveVelocity(200);
				}

				drive.arcade(leftY, rightX, .07); //Normal Drive code
		}else	if(btnB.isPressed()){//used to keep the cube in intake stationary
			 drive.arcade(leftY/3.5, rightX/3, 0.05);				//drive moves back
			 LeftIntake.moveVelocity(leftY*70);						//intake moves out
			 RightIntake.moveVelocity(leftY*70);					//2 motions cancel out and cube does not move

	 	}else{
			if(leftUp.isPressed()){					//Intake in
				LeftIntake.moveVelocity(100);
				RightIntake.moveVelocity(100);
			}else if(leftDown.isPressed()){	//Intake out
				LeftIntake.moveVelocity(-120);
				RightIntake.moveVelocity(-120);
			}else if(btnLeft.isPressed()){	//rotate cube
				LeftIntake.moveVelocity(-120);
				RightIntake.moveVelocity(120);
			}else if(rightUp.isPressed() && rightDown.isPressed()){
				intake(-30);
			}else{														//Intake off
				LeftIntake.moveVelocity(0);
				RightIntake.moveVelocity(0);
			}
			drive.arcade(leftY, rightX, .07); //Normal Drive code
		}

		if(btnA.isPressed()){							//ENTER DROPPER MODE
			if(rightUp.isPressed()){					//Dropper up
				//Dropper proportional Curve
				dropperError = (((dropperMax-Dropper.getPosition())/dropperMax))*200;
				try{
				dropperTargetSpeed = (int)((1.0/(1.0 + d_m*std::pow(e,((-1.0*dropperError)/d_d))))*(200-d_N)+d_N);
				}catch(std::exception e){
					text(2, e.what());
				dropperTargetSpeed=30;
				}
				text(3, "error: " + std::to_string(dropperError));
				text(4, "pow: " + std::to_string(dropperTargetSpeed));
				// Dropper.moveVelocity((((dropperMax-Dropper.getPosition())/dropperMax)*200)>50?(((3000-Dropper.getPosition())/2800)*200):50);
				Dropper.moveVelocity(dropperTargetSpeed);
			}else if(rightDown.isPressed()){//Dropper down
				Dropper.moveVelocity(-120);
			}else if(btnY.isPressed()){
 			 Dropper.moveVelocity(-200);
 		 }else{
				Dropper.moveVelocity(0);		//Dropper off
				intake(15);
			}

			if(leftUp.isPressed()){					//Intake in slowly
				LeftIntake.moveVelocity(30);
				RightIntake.moveVelocity(30);
			}else if(leftDown.isPressed()){	//Intake out slowly
				LeftIntake.moveVelocity(-30);
				RightIntake.moveVelocity(-30);
			}else{													//Intake off
				LeftIntake.moveVelocity(leftY<0?leftY*intakeMultiplier:0);
				RightIntake.moveVelocity(leftY<0?leftY*intakeMultiplier:0);
			}
			drive.arcade(leftY/3.5,leftX/5);//slow drive
			FourBar.moveVelocity(0);				//lift off
		}else{
			Dropper.moveVoltage(0);
		}																	//EXIT DROPPER mode

		if(btnB.changedToPressed()){
			if(Dropper.getPosition() > 2){
				bringDropperBack = true;
			}
			if(FourBar.getPosition() < -2){
				bringLiftBack = true;
			}
		}

		if(bringDropperBack){
			if(Dropper.getPosition() > 2){
				Dropper.moveVelocity(-200);
				intake(40);
			}else{
				Dropper.moveVelocity(0);
				intake(0);
				bringDropperBack = false;
			}
		}

		if(bringLiftBack){
			if(FourBar.getPosition() < -50){
				FourBar.moveVelocity(200);
			}else{
				FourBar.moveVelocity(0);
				bringLiftBack = false;
			}
		}


		text(0, std::to_string(FourBar.getPosition()));
     pros::delay(50);

		 if(isRecording){
			 LeftDriveBackSpeed = LeftDriveBack.getTargetVelocity();
			 LeftDriveFrontSpeed = LeftDriveFront.getTargetVelocity();
			 RightDriveBackSpeed = RightDriveBack.getTargetVelocity();
			 RightDriveFrontSpeed = RightDriveFront.getTargetVelocity();
			 LeftIntakeSpeed = LeftIntake.getTargetVelocity();
			 RightIntakeSpeed = RightIntake.getTargetVelocity();
			 FourBarSpeed = FourBar.getTargetVelocity();
			 DropperSpeed = Dropper.getTargetVelocity();

			 FILE* USDWriter = fopen("/usd/run1.txt", "a");

			 fprintf(USDWriter, "LeftDriveBack.moveVelocity(%i); \n", LeftDriveBackSpeed);
			 fprintf(USDWriter, "LeftDriveFront.moveVelocity(%i); \n", LeftDriveFrontSpeed);
			 fprintf(USDWriter, "RightDriveBack.moveVelocity(%i); \n", RightDriveBackSpeed);
			 fprintf(USDWriter, "RightDriveFront.moveVelocity(%i); \n", RightDriveFrontSpeed);
			 fprintf(USDWriter, "LeftIntake.moveVelocity(%i); \n", LeftIntakeSpeed);
			 fprintf(USDWriter, "RightIntake.moveVelocity(%i); \n", RightIntakeSpeed);
			 fprintf(USDWriter, "FourBar.moveVelocity(%i); \n", FourBarSpeed);
			 fprintf(USDWriter, "DropperSpeed.moveVelocity(%i); \n", DropperSpeed);

			 timeNew = pros::millis();
			 deltaTime = timeNew - timeOld;
			 timeOld = pros::millis();

			 fprintf(USDWriter, "pros::delay(%i); \n", deltaTime);
			 fclose(USDWriter);


		 }//auton recorder
   }//while loop
 }//op control


void goToHighTower(){
	while(FourBar.getPosition() < 500){
		FourBar.moveVelocity(200);
		leftY = controller.getAnalog(ControllerAnalog::leftY);
		rightX = controller.getAnalog(ControllerAnalog::rightX);
		drive.arcade(leftY, rightX, .07); //Normal Drive code
		pros::delay(50);
	}


}
