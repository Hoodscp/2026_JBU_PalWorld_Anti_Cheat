#pragma once

// 장비 내구도 / 무기 잔탄 강제 치트.
// 인벤토리 슬롯의 DynamicItemData(TWeakObjectPtr)를 GUObjectArray 로 해소해
// 자동으로 UPalDynamicArmor/WeaponItemDataBase 객체에 도달한 뒤 +0x78 Durability
// 와 +0x84 RemainingBullets 를 매 프레임 강제한다. Menu::Config 의
// SelectedContainerIdx / SelectedSlotIndex 를 그대로 공유한다.
namespace ItemDurabilityCheats
{
    void Tick();
}
