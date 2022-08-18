function init()
    local Lane = { 
        isEnabled = Type:Bool(),
        inputId = Type:Int32(),
        displayIndex = Type:Int32(),
        width = Type:Float(),
        showInHud = Type:Bool() }

    GLOBAL.iLanes = {
        neighborLeft = Lane,
        ego = Lane,
        neighborRight = Lane }

    GLOBAL.iAcc = {  
        state = Type:Int32(),                    --enum: Off, Active, Overruled, TakeOverRequest, StandBy
        followingState = Type:Int32(),           --enum: Not_Following, Following, RequestStartDriving, Overruled, TakeOverRequest  
        handsOffOptionState = Type:Int32(),      --enum: Not_Available, Available, Active, StandBy_Hack
        desiredDistanceSteps = Type:Int32(),     --enum: Invalid, One_Step, Two_Steps, Three_Steps, Four_Steps
        desiredDistance = Type:Float(),       
        desiredDistanceStepsButtonPressedCounter = Type:Int32() }

    GLOBAL.iLsa = {  
        maneuver = Type:Int32(),                 -- enum: NoManeuver, LaneChangeLeft, LaneChangeRight, LaneChangeSearchLeft, LaneChangeSearchRight
        laneMarkingDisplayLeft = Type:Int32(),   -- enum: NotVisible, Visible, Visible_Reduced, Warning_Lane_Change, Warning_Lane_Departure
        laneMarkingDisplayRight = Type:Int32(),  -- enum: NotVisible, Visible, Visible_Reduced, Warning_Lane_Change, Warning_Lane_Departure
        laneMarkingLaneInputIdLeft = Type:Int32(),
        laneMarkingLaneDisplayIndexLeft = Type:Int32(),
        laneMarkingLaneInputIdRight = Type:Int32(),
        laneMarkingLaneDisplayIndexRight = Type:Int32(),
        lsaDropCounter = Type:Int32() }

    GLOBAL.iEmergencyCorridor = {
        laneInputId = Type:Int32(),
        laneDisplayIndex = Type:Int32(),
        positionY = Type:Float(),
        width = Type:Float() }    

    GLOBAL.iRabbit = {        
        isEnabled = Type:Bool(),
        positionX = Type:Float(),
        positionY = Type:Float(),
        rawReferencePointX = Type:Float(),
        rawReferencePointY = Type:Float(),
        laneInputId = Type:Int32(),
        laneDisplayIndex = Type:Int32(),
        absoluteVelocityX = Type:Float(),
        objectClassification = Type:Int32(),         -- enum: Unknown, Bike, Car, Truck, Van, Pedestrian
        classificationLocked = Type:Bool(),
        objectHighlightReason = Type:Int32(),        -- enum: None, Following, RequestStartDriving, Overruled, TakeOverRequest, Warning
        objectInputId = Type:Int32(),
        objectDisplayId = Type:Int32(),
        length = Type:Float(),
        width = Type:Float(),
        height = Type:Float(),
        noLaneChangeAnimation = Type:Bool(),
        keepInHudHighlight = Type:Bool(),
        showInHud = Type:Bool() }

    GLOBAL.iLaneChangeHint = {   
        objectDisplayIdFront = Type:Int32(),
        objectDisplayIdBack = Type:Int32(),
        gapLaneInputId = Type:Int32(),
        gapLaneDisplayIndex = Type:Int32() }      
        
    GLOBAL.iEgoVehicle = {
        positionY = Type:Float(),
        absoluteVelocityX = Type:Float(),
        inputLaneId = Type:Int32(),
        laneDisplayIndex = Type:Int32() }

    local TimeStamp = {
        isValid = Type:Bool(),
        seconds = Type:Int32(),
        nanoseconds = Type:Int32() }

    GLOBAL.iEgoVehicleConfig = {
        length = Type:Float(),
        width = Type:Float() }

    GLOBAL.iSceneMeta = {
        --scene and display behaviour   
        timeStamp = TimeStamp,
        beginProcessingTimeStamp = TimeStamp,    -- unsure if needed
        operatingState = Type:Int32(),                   -- enum: NotAvailable_Initializing, NotAvailable_GenericError, Available
        displayState = Type:Int32(),                     -- enum: Do_Not_Display, Display, Display_Reduced

        --SP21
        drivingModeLevel = Type:Int32(),                  -- enum: Level0_Manual, Level1_TAF, Level2_HAF
        isLivingMode = Type:Bool(),
        focusRequestPossible = Type:Bool(),
        isTrailerActive = Type:Bool() }

end

function interface(IN,OUT)     
    --scene object controller inputs
    IN.ACC = GLOBAL.iAcc
    IN.LSA = GLOBAL.iLsa
    IN.LaneChangeHint = GLOBAL.iLaneChangeHint
    IN.Lanes = GLOBAL.iLanes
    IN.DetectedObjects = Type:Array(14, GLOBAL.iRabbit)
    IN.EgoVehicle = GLOBAL.iEgoVehicle
    IN.EmergencyCorridor = GLOBAL.iEmergencyCorridor    
    IN.EgoVehicleConfig = GLOBAL.iEgoVehicleConfig
    IN.SceneMeta = GLOBAL.iSceneMeta
    
    --output
    OUT.ACC = GLOBAL.iAcc
    OUT.LSA = GLOBAL.iLsa
    OUT.LaneChangeHint = GLOBAL.iLaneChangeHint
    OUT.Lanes = GLOBAL.iLanes
    OUT.DetectedObjects = Type:Array(14, GLOBAL.iRabbit)
    OUT.EgoVehicle = GLOBAL.iEgoVehicle
    OUT.EmergencyCorridor = GLOBAL.iEmergencyCorridor    
    OUT.EgoVehicleConfig = GLOBAL.iEgoVehicleConfig
    OUT.SceneMeta = GLOBAL.iSceneMeta
end

function run(IN,OUT)
  
  --pass through
  OUT.ACC = IN.ACC
  OUT.LSA = IN.LSA
  OUT.LaneChangeHint = IN.LaneChangeHint
  OUT.Lanes = IN.Lanes
  OUT.DetectedObjects = IN.DetectedObjects
  OUT.EgoVehicle = IN.EgoVehicle
  OUT.EmergencyCorridor = IN.EmergencyCorridor    
  OUT.EgoVehicleConfig = IN.EgoVehicleConfig
  OUT.SceneMeta = IN.SceneMeta
  
end