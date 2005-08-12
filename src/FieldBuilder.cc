#include "DetectorDescription/Parser/interface/DDLParser.h"
#include "DetectorDescription/Core/interface/DDLogicalPart.h"

#include "SimG4Core/MagneticField/interface/FieldBuilder.h"
#include "SimG4Core/MagneticField/interface/Field.h"
#include "SimG4Core/MagneticField/interface/FieldStepper.h"
#include "SimG4Core/Geometry/interface/G4LogicalVolumeToDDLogicalPartMapper.h"
#include "SimG4Core/Notification/interface/SimG4Exception.h"

#include "G4Mag_UsualEqRhs.hh"
#include "G4PropagatorInField.hh"
#include "G4FieldManager.hh"
#include "G4TransportationManager.hh"
#include "G4ChordFinder.hh"
#include "G4UniformMagField.hh"

FieldBuilder * FieldBuilder::theBuilder = 0;
FieldBuilder * FieldBuilder::instance()
{
    if (theBuilder==0) theBuilder = new FieldBuilder();
    return theBuilder;
}

FieldBuilder::FieldBuilder(): theField(0), theTopVolume(0) {}
FieldBuilder::~FieldBuilder() { if (theField != 0) delete theField; }

void FieldBuilder::readFieldParameters(DDLogicalPart lp,std::string keywordField)
{
    G4LogicalVolumeToDDLogicalPartMapper * m = 
	G4LogicalVolumeToDDLogicalPartMapper::instance();
    int tmp;
    tmp = m->toString(keywordField,lp,fieldType);
    tmp = m->toDouble("FieldValue",lp,fieldValue);
    tmp = m->toString("Stepper",lp,stepper);
    tmp = m->toDouble("MinStep",lp,minStep);
    tmp = m->toDouble("DeltaChord",lp,dChord);
    tmp = m->toDouble("DeltaOneStep",lp,dOneStep);
    tmp = m->toDouble("DeltaIntersection",lp,dIntersection);
    tmp = m->toDouble("DeltaIntersectionAndOneStep",lp,dIntersectionAndOneStep);
    tmp = m->toDouble("MaximumLoopCount",lp,maxLoopCount);
    tmp = m->toDouble("MinimumEpsilonStep",lp,minEpsilonStep);
    tmp = m->toDouble("MaximumEpsilonStep",lp,maxEpsilonStep);
}

void FieldBuilder::setField()
{
    G4MagneticField * f = new G4UniformMagField(Hep3Vector(0.,0.,4*tesla));
    theField = f;    
    theFieldEquation = new G4Mag_UsualEqRhs(theField);
    theField->fieldEquation(theFieldEquation);
}

void FieldBuilder::configure(std::string keywordField,G4FieldManager * fM,
				   G4PropagatorInField * fP)
{
    ConcreteG4LogicalVolumeToDDLogicalPartMapper::Vector vec = 
        G4LogicalVolumeToDDLogicalPartMapper::instance()->all(keywordField);
    for (ConcreteG4LogicalVolumeToDDLogicalPartMapper::Vector::iterator 
	     tit = vec.begin(); tit != vec.end(); tit++)            
    {
	theTopVolume = (*tit).first;
	readFieldParameters((*tit).second,keywordField);	
	if (fM!=0) configureFieldManager(fM);
	if (fP!=0) configurePropagatorInField(fP);	
    }
}

void FieldBuilder::configureFieldManager(G4FieldManager * fM)
{
    if (fM==0) return;
    fM->SetDetectorField(Field::instance());
    FieldStepper * theStepper = new FieldStepper(Field::fieldEquation());
    theStepper->select(stepper);
    G4ChordFinder * CF = new G4ChordFinder(Field::instance(),minStep,theStepper);
    CF->SetDeltaChord(dChord);
    fM->SetChordFinder(CF);
    fM->SetDeltaOneStep(dOneStep);
    fM->SetDeltaIntersection(dIntersection);
    if (dIntersectionAndOneStep != -1.) 
	fM->SetAccuraciesWithDeltaOneStep(dIntersectionAndOneStep);
}

void FieldBuilder::configurePropagatorInField(G4PropagatorInField * fP)
{
    if (fP==0) return;
    fP->SetMaxLoopCount(int(maxLoopCount));
    fP->SetMinimumEpsilonStep(minEpsilonStep);
    fP->SetMaximumEpsilonStep(maxEpsilonStep);
    fP->SetVerboseLevel(0);
}

G4LogicalVolume * FieldBuilder::fieldTopVolume() { return theTopVolume; }