within ;
package Modelica_introduction

  package Part01_Dymola
    model Model_01
      Modelica.Blocks.Sources.Sine sine
        annotation (Placement(transformation(extent={{-48,-18},{-12,18}})));
      Modelica.Blocks.Math.Gain gain(k=2)
        annotation (Placement(transformation(extent={{12,-10},{32,10}})));
    equation
      connect(gain.u, sine.y)
        annotation (Line(points={{10,0},{-10.2,0}}, color={0,0,127}));
      annotation (Icon(coordinateSystem(preserveAspectRatio=false)), Diagram(
            coordinateSystem(preserveAspectRatio=false)));
    end Model_01;

    model ThreeTanks "Demonstrating the usage of SimpleTank"
      import Modelica.Fluid;
      extends Modelica.Icons.Example;
       // replaceable package Medium = Modelica.Fluid.Media.Water.ConstantPropertyLiquidWater extends
      // replaceable package Medium = Modelica.Media.Water.StandardWaterOnePhase extends
      // replaceable package Medium = Modelica.Media.Incompressible.Examples.Glycol47 extends
       replaceable package Medium =
          Modelica.Media.Water.ConstantPropertyLiquidWater                           constrainedby
        Modelica.Media.Interfaces.PartialMedium "Medium in the component"
          annotation (choicesAllMatching = true);

      Modelica.Fluid.Vessels.OpenTank tank1(
        crossArea=1,
        redeclare package Medium = Medium,
        use_portsData=true,
        height=12,
        level_start=12,
        nPorts=1,
        portsData={Modelica.Fluid.Vessels.BaseClasses.VesselPortsData(diameter=
            0.1)}) annotation (Placement(transformation(extent={{-80,20},{-40,
                60}})));
      Modelica.Fluid.Vessels.OpenTank tank2(
        crossArea=1,
        redeclare package Medium = Medium,
        use_portsData=true,
        height=12,
        level_start=3,
        nPorts=1,
        portsData={Modelica.Fluid.Vessels.BaseClasses.VesselPortsData(diameter=
            0.1)}) annotation (Placement(transformation(extent={{-20,20},{20,
                60}})));

      inner Modelica.Fluid.System system(energyDynamics=Modelica.Fluid.Types.Dynamics.FixedInitial)
                                       annotation (Placement(transformation(
              extent={{70,-90},{90,-70}})));
      Modelica.Fluid.Vessels.OpenTank tank3(
        crossArea=1,
        redeclare package Medium = Medium,
        use_portsData=true,
        height=2,
        level_start=0,
        nPorts=1,
        portsData={Modelica.Fluid.Vessels.BaseClasses.VesselPortsData(diameter=
            0.1)}) annotation (Placement(transformation(extent={{40,10},{80,
                50}})));
      Modelica.Fluid.Pipes.StaticPipe pipe1(                    redeclare package
          Medium =                                                                       Medium,
        allowFlowReversal=true,
        height_ab=2,
        length=2,
        diameter=0.1)
                     annotation (Placement(transformation(
            origin={-60,-10},
            extent={{-10,-10},{10,10}},
            rotation=90)));
      Modelica.Fluid.Pipes.StaticPipe pipe2(                    redeclare package
          Medium =                                                                       Medium,
        allowFlowReversal=true,
        height_ab=2,
        length=2,
        diameter=0.1)
                     annotation (Placement(transformation(
            origin={0,-10},
            extent={{-10,-10},{10,10}},
            rotation=90)));
      Modelica.Fluid.Pipes.StaticPipe pipe3(                    redeclare package
          Medium =                                                                       Medium,
        allowFlowReversal=true,
        height_ab=-1,
        length=2,
        diameter=0.1) annotation (Placement(transformation(
            origin={60,-20},
            extent={{-10,-10},{10,10}},
            rotation=90)));
    equation
      connect(pipe1.port_a, pipe2.port_a) annotation (Line(points={{-60,-20},{-60,
              -40},{0,-40},{0,-30},{0,-20}}, color={0,127,255}));
      connect(pipe2.port_a, pipe3.port_a) annotation (Line(points={{0,-20},{0,-20},
              {0,-40},{60,-40},{60,-30}}, color={0,127,255}));
      connect(pipe3.port_b, tank3.ports[1])
        annotation (Line(points={{60,-10},{60,-10},{60,10}}, color={0,127,255}));
      connect(pipe1.port_b, tank1.ports[1]) annotation (Line(points={{-60,0},{
              -60,20}},      color={0,127,255}));
      connect(pipe2.port_b, tank2.ports[1]) annotation (Line(
          points={{0,0},{0,20}}, color={0,127,255}));

      annotation (experiment(StopTime=200),
        __Dymola_Commands(file=
              "modelica://Modelica/Resources/Scripts/Dymola/Fluid/ThreeTanks/plot level and port.m_flow.mos"
            "plot level and port.m_flow"),
        Documentation(info="<html>

<img src=\"modelica://Modelica/Resources/Images/Fluid/Examples/Tanks/ThreeTanks.png\" border=\"1\"
     alt=\"ThreeTanks.png\">
</html>"));
    end ThreeTanks;
  end Part01_Dymola;

  package Part02_Equations
    model Model_01
      Real G1 "Thermal conductance layer 1";
      Real G2 "Thermal conductance layer 2";
      Real T1 "Temperature left";
      Real T2 "Temperature right";
      Real Ti "Temperature interface";
      Real Q_flow "Heat flow rate";
    algorithm
    // Parameters
      G1 := 1;
      G2 := 2;
    // Boundary conditions
      Q_flow := 100;
      T2 := 0;
    // Heat conduction
      Ti := Q_flow*G2+T2;
      T1 := Q_flow*G1+Ti;
    end Model_01;

    model Model_02
      Real G1 "Thermal conductance layer 1";
      Real G2 "Thermal conductance layer 2";
      Real T1 "Temperature left";
      Real T2 "Temperature right";
      Real Ti "Temperature interface";
      Real Q_flow "Heat flow rate";
    algorithm
      // Parameters
      G1 := 1;
      G2 := 2;
      // Boundary conditions
      T1 := 150;
      T2 := 0;
      // Heat conduction
      Ti := (G1*T1 + G2*T2)/(G1+G2);
      Q_flow := G1*(T1 - Ti);
    end Model_02;

    model Model_03
      Real G1 "Thermal conductance layer 1";
      Real G2 "Thermal conductance layer 2";
      Real T1 "Temperature left";
      Real T2 "Temperature right";
      Real Ti "Temperature interface";
      Real Q_flow "Heat flow rate";
    equation
      // Parameters
      G1 =  1;
      G2 =  2;
      // Boundary conditions
      T1 =  150;
      T2 =  0;
      // Heat conduction
      Q_flow = G1*(T1-Ti);
      Q_flow = G2*(Ti-T2);
    end Model_03;

    model Model_04
      parameter Real G1 = 1 "Thermal conductance layer 1";
      parameter Real G2 = 2 "Thermal conductance layer 2";
      Real T1 "Temperature left";
      Real T2 "Temperature right";
      Real Ti "Temperature interface";
      Real Q_flow "Heat flow rate";
    equation
      // Boundary conditions
      T1 =  150;
      T2 =  0;
      // Heat conduction
      Q_flow = G1*(T1-Ti);
      Q_flow = G2*(Ti-T2);
    end Model_04;

    model Model_05
      import Modelica.Units.SI;
      parameter SI.ThermalConductance G1 = 1 "Thermal conductance layer 1";
      parameter SI.ThermalConductance G2 = 2 "Thermal conductance layer 2";
      SI.Temperature T1 "Temperature left";
      SI.Temperature T2 "Temperature right";
      SI.Temperature Ti "Temperature interface";
      SI.HeatFlowRate Q_flow "Heat flow rate";
    equation
      // Boundary conditions
      T1 =  273.15 + 150;
      T2 =  273.15 + 0;
      // Heat conduction
      Q_flow = G1*(T1-Ti);
      Q_flow = G2*(Ti-T2);
    end Model_05;
  end Part02_Equations;
  annotation (uses(Modelica(version="4.0.0")));
end Modelica_introduction;
