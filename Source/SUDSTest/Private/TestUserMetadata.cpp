#include "SUDSDialogue.h"
#include "SUDSLibrary.h"
#include "SUDSMessageLogger.h"
#include "SUDSScript.h"
#include "SUDSScriptImporter.h"
#include "TestUtils.h"
#include "Misc/AutomationTest.h"

UE_DISABLE_OPTIMIZATION

// Assign known keys to the strings so we can detect 
const FString UserMetadataInput = R"RAWSUD(
# No metadata
Player: Good day sir
#% IsGreeting = true
NPC: Salutations to you too
#% IntValue = 1
#% Politeness 3.142
Player: What are we doing today?
# Test a variable-derived, expression metadata
[set IntVariable 2]
#% IntValue = {IntVariable} + 3
#% LookMumNoEquals `OK_Dear`
NPC: Looks like we're doing some testing. How about a choice?
  #% RequirePoshness = 10
  * Choice 1
     Player: Capital, old chap
  #% RequirePoshness = 15
  #% RequireTopHat = true
  * Choice 2
     Player: Top hole, dear boy
  * Choice 3
     Player: OK then
)RAWSUD";

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestUserMetadata,
								 "SUDSTest.TestUserMetadata",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)



bool FTestUserMetadata::RunTest(const FString& Parameters)
{
	FSUDSMessageLogger Logger(false);
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(UserMetadataInput), UserMetadataInput.Len(), "UserMetadataInput", &Logger, true));

	auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
	const ScopedStringTableHolder StringTableHolder;
	Importer.PopulateAsset(Script, StringTableHolder.StringTable);

	// Script shouldn't be the owner of the dialogue but it's the only UObject we've got right now so why not
	auto Dlg = USUDSLibrary::CreateDialogue(Script, Script);
	
	Dlg->Start();

	TestDialogueText(this, "First node", Dlg, "Player", "Good day sir");
	TestEqual("Check metadata empty", Dlg->GetAllSpeakerLineUserMetadata().Num(), 0);

	Dlg->Continue();
	TestDialogueText(this, "Next", Dlg, "NPC", "Salutations to you too");
	FSUDSValue Actual = Dlg->GetSpeakerLineUserMetadata(FName("IsGreeting"));
	if (TestTrue("Boolean test", Actual.GetType() == ESUDSValueType::Boolean))
	{
		TestEqual("Boolean test", Actual.GetBooleanValue(), true);
	}
	
	Dlg->Continue();
	TestDialogueText(this, "Next", Dlg, "Player", "What are we doing today?");
	Actual = Dlg->GetSpeakerLineUserMetadata(FName("IntValue"));
	if (TestTrue("Int test", Actual.GetType() == ESUDSValueType::Int))
	{
		TestEqual("Int test", Actual.GetIntValue(), 1);
	}
	Actual = Dlg->GetSpeakerLineUserMetadata(FName("Politeness"));
	if (TestTrue("Float test", Actual.GetType() == ESUDSValueType::Float))
	{
		TestEqual("Float test", Actual.GetFloatValue(), 3.142f);
	}
	Dlg->Continue();
	TestDialogueText(this, "Next", Dlg, "NPC", "Looks like we're doing some testing. How about a choice?");
	Actual = Dlg->GetSpeakerLineUserMetadata(FName("IntValue"));
	if (TestTrue("Expression test", Actual.GetType() == ESUDSValueType::Int))
	{
		TestEqual("Expression test", Actual.GetIntValue(), 5);
	}
	Actual = Dlg->GetSpeakerLineUserMetadata(FName("LookMumNoEquals"));
	if (TestTrue("Name & no equals test", Actual.GetType() == ESUDSValueType::Name))
	{
		TestEqual("Name & no equals test", Actual.GetNameValue(), FName("OK_Dear"));
	}

	if (TestEqual("Choice count", Dlg->GetNumberOfChoices(), 3))
	{
		Actual = Dlg->GetChoiceUserMetadata(0, FName("RequirePoshness"));
		if (TestTrue("Choice 0 test", Actual.GetType() == ESUDSValueType::Int))
		{
			TestEqual("Choice 0 test", Actual.GetIntValue(), 10);
		}
		Actual = Dlg->GetChoiceUserMetadata(1, FName("RequirePoshness"));
		if (TestTrue("Choice 1 test A", Actual.GetType() == ESUDSValueType::Int))
		{
			TestEqual("Choice 1 test A", Actual.GetIntValue(), 15);
		}
		Actual = Dlg->GetChoiceUserMetadata(1, FName("RequireTopHat"));
		if (TestTrue("Choice 1 test B", Actual.GetType() == ESUDSValueType::Boolean))
		{
			TestEqual("Choice 1 test B", Actual.GetBooleanValue(), true);
		}
		auto Choice1All = Dlg->GetAllChoiceUserMetadata(1);
		TestEqual("Choice 1 test count", Choice1All.Num(), 2);
		TestEqual("Choice 2 test", Dlg->GetAllChoiceUserMetadata(2).Num(), 0);
		
	}


	Script->MarkAsGarbage();
	return true;
	
}

UE_ENABLE_OPTIMIZATION
