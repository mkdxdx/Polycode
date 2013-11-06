/*
 Copyright (C) 2012 by Ivan Safrin
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */
 
#include "PolycodeEntityEditor.h"
#include "PolycodeFrame.h"

extern UIGlobalMenu *globalMenu;
extern PolycodeFrame *globalFrame;

EntityEditorMainView::EntityEditorMainView() {
	processInputEvents = true;

	mainScene = new Scene(Scene::SCENE_3D, true);		
	renderTexture = new SceneRenderTexture(mainScene, mainScene->getDefaultCamera(), 512, 512);
	mainScene->clearColor.setColor(0.2, 0.2, 0.2, 1.0);	
	mainScene->useClearColor = true;
	mainScene->rootEntity.processInputEvents = true;
	
	renderTextureShape = new UIRect(256, 256);
	renderTextureShape->setAnchorPoint(-1.0, -1.0, 0.0);	
	renderTextureShape->setTexture(renderTexture->getTargetTexture());
	addChild(renderTextureShape);
	renderTextureShape->setPosition(0, 30);
			
	headerBg = new UIRect(10,10);
	addChild(headerBg);
	headerBg->setAnchorPoint(-1.0, -1.0, 0.0);
	headerBg->color.setColorHexFromString(CoreServices::getInstance()->getConfig()->getStringValue("Polycode", "uiHeaderBgColor"));
    
	
	sideBar = new Entity();
	addChild(sideBar);
	sideBar->setPosition(0, 0);
	sideBar->processInputEvents = true;
    
	
	mainScene->getDefaultCamera()->setPosition(10, 10, 10);
	mainScene->getDefaultCamera()->lookAt(Vector3());

	grid = new EditorGrid();
	mainScene->addChild(grid);
	
    sceneObjectRoot = new Entity();
    sceneObjectRoot->processInputEvents = true;
    mainScene->addChild(sceneObjectRoot);
    
	transformGizmo = new TransformGizmo(mainScene, mainScene->getDefaultCamera());
	mainScene->addChild(transformGizmo);		
	trackballCamera = new TrackballCamera(mainScene->getDefaultCamera(), renderTextureShape);
	
    addEntityButton = new UIImageButton("entityEditor/add_entity.png", 1.0, 24, 24);
	sideBar->addChild(addEntityButton);
    addEntityButton->setPosition(4, 2);
    addEntityButton->addEventListener(this, UIEvent::CLICK_EVENT);
    
	transformGizmoMenu = new TransformGizmoMenu(transformGizmo);
	sideBar->addChild(transformGizmoMenu);
    transformGizmoMenu->setPositionX(40);
    
}

Entity *EntityEditorMainView::getSelectedEntity() {
    if(selectedEntities.size() > 0) {
        return selectedEntities[selectedEntities.size()-1];
    } else {
        return NULL;
    }
}

void EntityEditorMainView::Update() {
    for(int i=0; i < icons.size(); i++) {
        Number scale = mainScene->getDefaultCamera()->getPosition().distance(icons[i]->getConcatenatedMatrix().getPosition()) * 0.1;
        icons[i]->setScale(scale, scale, scale);
    }
}

void EntityEditorMainView::createIcon(Entity *entity, String iconFile) {
    ScenePrimitive *iconPrimitive = new ScenePrimitive(ScenePrimitive::TYPE_VPLANE, 0.3, 0.3);
    
    iconPrimitive->setMaterialByName("UnlitMaterial");
	Texture *tex = CoreServices::getInstance()->getMaterialManager()->createTextureFromFile("entityEditor/"+iconFile);
	if(iconPrimitive->getLocalShaderOptions()) {
        iconPrimitive->getLocalShaderOptions()->addTexture("diffuse", tex);
	}
    
    entity->addChild(iconPrimitive);
    iconPrimitive->billboardMode = true;
    iconPrimitive->depthTest = false;
    iconPrimitive->depthWrite = false;
    iconPrimitive->editorOnly = true;
    iconPrimitive->alphaTest = true;
    icons.push_back(iconPrimitive);
}

void EntityEditorMainView::setEditorProps(Entity *entity) {
    entity->processInputEvents = true;
    entity->addEventListener(this, InputEvent::EVENT_MOUSEDOWN);
    
    SceneMesh *sceneMesh = dynamic_cast<SceneMesh*>(entity);
    if(sceneMesh) {
        sceneMesh->wireFrameColor = Color(1.0, 0.8, 0.3, 1.0);
        sceneMesh->setLineWidth(CoreServices::getInstance()->getRenderer()->getBackingResolutionScaleX());
        sceneMesh->useGeometryHitDetection = true;
    }
    
    SceneLight *sceneLight = dynamic_cast<SceneLight*>(entity);
    if(sceneLight) {
        createIcon(entity, "light_icon.png");
    }
}

void EntityEditorMainView::addEntityFromMenu(String command) {

    if(command == "add_primitive") {
        ScenePrimitive  *newPrimitive = new ScenePrimitive(ScenePrimitive::TYPE_BOX, 1.0, 1.0, 1.0);
        sceneObjectRoot->addChild(newPrimitive);
        setEditorProps(newPrimitive);
        newPrimitive->setPosition(cursorPosition);
        selectEntity(newPrimitive);
        newPrimitive->getMesh()->calculateNormals(false);
        return;
    }

    if(command == "add_light") {
        SceneLight *newLight = new SceneLight(SceneLight::AREA_LIGHT, mainScene, 50);
        
        newLight->bBox = Vector3(0.5, 0.5, 0.5);
        mainScene->addLight(newLight);
        sceneObjectRoot->addChild(newLight);
        setEditorProps(newLight);
        newLight->setPosition(cursorPosition);
        selectEntity(newLight);
        return;
    }
    
    if(command == "add_mesh") {
        assetSelectType = "mesh";
        globalFrame->assetBrowser->addEventListener(this, UIEvent::OK_EVENT);
        std::vector<String> extensions;
        extensions.push_back("mesh");
        globalFrame->showAssetBrowser(extensions);
        return;
    }

}

void EntityEditorMainView::handleEvent(Event *event) {

    if(event->getDispatcher() == globalFrame->assetBrowser) {
        if(event->getEventCode() == UIEvent::OK_EVENT) {
            if(assetSelectType == "mesh") {
                SceneMesh *newMesh = new SceneMesh(globalFrame->assetBrowser->getFullSelectedAssetPath());
                sceneObjectRoot->addChild(newMesh);
                setEditorProps(newMesh);
                newMesh->setPosition(cursorPosition);
                selectEntity(newMesh);
            }
            
            globalFrame->assetBrowser->removeAllHandlersForListener(this);
            globalFrame->hideModal();
        }
        
    } else if(event->getDispatcher() == addEntityMenu) {
        addEntityMenu->removeAllHandlersForListener(this);
        String command = addEntityMenu->getSelectedItem()->getMenuItemID();
        addEntityFromMenu(command);
    } else if(event->getDispatcher() == addEntityButton) {
        addEntityMenu = globalMenu->showMenuAtMouse(150);
        addEntityMenu->addOption("Add Primitive", "add_primitive");
        addEntityMenu->addOption("Add Mesh", "add_mesh");
        addEntityMenu->addOption("Add Entity", "add_entity");
        addEntityMenu->addOption("Add Sprite", "add_sprite");
        addEntityMenu->addOption("Add Label", "add_label");
        addEntityMenu->addDivider();
        addEntityMenu->addOption("Add Light", "add_light");
        addEntityMenu->addOption("Add Particle System", "add_particles");
        addEntityMenu->addOption("Add Sound", "add_sound");
        addEntityMenu->addOption("Add Camera", "add_camera");
        addEntityMenu->addDivider();
        addEntityMenu->addOption("Add Empty", "add_empty");
        addEntityMenu->fitToScreenVertical();
        addEntityMenu->addEventListener(this, UIEvent::OK_EVENT);
    } else {
        if(event->getEventCode() == InputEvent::EVENT_MOUSEDOWN ) {
            InputEvent *inputEvent = (InputEvent*) event;

            if(inputEvent->mouseButton == CoreInput::MOUSE_BUTTON2) {
                Entity* targetEntity = (Entity*) event->getDispatcher();
                selectEntity(targetEntity);
            }
        }
    }
}

void EntityEditorMainView::selectEntity(Entity *targetEntity) {
    for(int i=0; i < selectedEntities.size(); i++) {
        SceneMesh *sceneMesh = dynamic_cast<SceneMesh*>(selectedEntities[i]);
        if(sceneMesh) {
            sceneMesh->overlayWireframe = false;
        }
    }
    
    selectedEntities.clear();
    selectedEntities.push_back(targetEntity);
    transformGizmo->setTransformSelection(selectedEntities);
    dispatchEvent(new Event(), Event::CHANGE_EVENT);
    
    SceneMesh *sceneMesh = dynamic_cast<SceneMesh*>(targetEntity);
    if(sceneMesh) {
        sceneMesh->overlayWireframe = true;
    }
    
}

EntityEditorMainView::~EntityEditorMainView() {
	
}

void EntityEditorMainView::Resize(Number width, Number height) {
	headerBg->Resize(width, 30);
	
	mainScene->sceneMouseAdjust.x = renderTextureShape->getScreenPositionForMainCamera().x;
	mainScene->sceneMouseAdjust.y = CoreServices::getInstance()->getCore()->getYRes() - (renderTextureShape->getScreenPositionForMainCamera().y + renderTextureShape->getHeight());
	
	renderTexture->resizeRenderTexture(width, height-30);
	renderTextureShape->setTexture(renderTexture->getTargetTexture());		
	renderTextureShape->Resize(width, height-30);
}

PolycodeEntityEditor::PolycodeEntityEditor() : PolycodeEditor(true){
	mainSizer = new UIHSizer(300, 300, 300, false);
	addChild(mainSizer);
	
	mainView = new EntityEditorMainView();
    mainView->addEventListener(this, Event::CHANGE_EVENT);
	mainSizer->addLeftChild(mainView);
    
    rightSizer = new UIVSizer(10, 10, 150, true);
    mainSizer->addRightChild(rightSizer);
    
    propertyView = new EntityEditorPropertyView();
    rightSizer->addBottomChild(propertyView);
}

void PolycodeEntityEditor::handleEvent(Event *event) {
    
    if(event->getDispatcher() == mainView) {
        switch(event->getEventCode()) {
            case Event::CHANGE_EVENT:
                propertyView->setEntity(mainView->getSelectedEntity());
            break;
        }
    }
    
    PolycodeEditor::handleEvent(event);
}

PolycodeEntityEditor::~PolycodeEntityEditor() {
}

bool PolycodeEntityEditor::openFile(OSFileEntry filePath) {	
	PolycodeEditor::openFile(filePath);	
	return true;
}

void PolycodeEntityEditor::Resize(int x, int y) {
	mainSizer->Resize(x, y);
	PolycodeEditor::Resize(x,y);
}

