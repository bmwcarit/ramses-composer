modules("anim_utils")


-- Define Input and Output parameters
function interface(IN,OUT)
    -- Input Parameters
    IN.extra_property = Type:Int32()
    IN.Extra_Struct = {
        extra_child1 = Type:Bool(),
        extra_child2 = Type:Int32()
    }

    IN.CameraPerspective_ID = Type:Int32() -- ID of camera perspective (DEFAULT: 1) - all options listed below
    --[[
        1: Side
        2: Wheel close-up
        3: Classic beauty shot
        4: Big front
        5: High roof
        6: Trunk
        7: Low Back Left
    ]]--

    IN.Camera_TurntableSettings = anim_utils.getTurntableSettings()

    IN.CarPaint_ID = Type:Int32() -- ID of exterior paint (DEFAULT: 1) - all options listed below
    --[[
        1: Pyhtonic Blue
        2: Transanit Blue
        3: Black
        4: Black Metallic
        5: Mineral White
        6: Silver Blue
        7: Ametrin Metallic
    ]]--

    -- Output Parameters
    OUT.extra_property = Type:Int32()
    OUT.Extra_Struct = {
        extra_child1 = Type:Bool(),
        extra_child2 = Type:Int32()
    }

    OUT.CameraPerspective_ID = Type:Int32() -- linked to 'Anim_CameraCrane' script
    OUT.Camera_TurntableSettings = anim_utils.getTurntableSettings() -- linked to 'Anim_CameraCrane' script

    OUT.CarPaint_ID = Type:Int32() -- linked to 'Anim_CarPaint' script

    IN.Door_B_L_isOpen = Type:Bool() -- Open/close door (back left) (false = fully closed, true = fully open)
    IN.Door_B_R_isOpen = Type:Bool() -- Open/close door (back right) (false = fully closed, true = fully open)
    IN.Door_F_L_isOpen = Type:Bool() -- Open/close door (front left) (false = fully closed, true = fully open)
    IN.Door_F_R_isOpen = Type:Bool() -- Open/close door (front right) (false = fully closed, true = fully open)
    IN.Tailgate_isOpen = Type:Bool() -- Open/close tailgate (false = fully closed, true = fully open)

    OUT.Door_B_L_isOpen = Type:Bool()
    OUT.Door_B_R_isOpen = Type:Bool()
    OUT.Door_F_L_isOpen = Type:Bool()
    OUT.Door_F_R_isOpen = Type:Bool()
    OUT.Tailgate_isOpen = Type:Bool()
end


-- Passing through the Input parameters to the Output parameters
function run(IN,OUT)
    OUT.extra_property = IN.extra_property
    OUT.Extra_Struct = IN.Extra_Struct

    OUT.CameraPerspective_ID = IN.CameraPerspective_ID
    OUT.CarPaint_ID = IN.CarPaint_ID
    OUT.Camera_TurntableSettings = IN.Camera_TurntableSettings

    OUT.Door_B_L_isOpen = IN.Door_B_L_isOpen
    OUT.Door_B_R_isOpen = IN.Door_B_R_isOpen
    OUT.Door_F_L_isOpen = IN.Door_F_L_isOpen
    OUT.Door_F_R_isOpen = IN.Door_F_R_isOpen
    OUT.Tailgate_isOpen = IN.Tailgate_isOpen
end
