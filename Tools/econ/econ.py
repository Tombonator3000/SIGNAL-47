# M7 pakke C: regnemodell for Freeform dag 1-30. Tall fra kilden (9. november):
#  S47Freeform.cpp:135 Cash=120; S47FreeformJobs.h:17 DailyCount=3; S47Freeform.h:159 PayAfterDays 20/20/15/10/5;
#  S47Freeform.h:161-163 ReviewPower 25, ReviewFuel 15, ArrearsCap 80, WearCap 15 (1 per 2 km), FilmCap 10 (2 per print), FeeCap 15 (3 per
#  poor recording); :169 StandbyFor = ceil(timer/3) maks 4 per hvil; StepWeeklyReview :2088 (uke = (dag-1)//7, gjennomgang dag 8, 15, 22, 29);
#  Earn :1120 betaler bare Loan; S47FieldLife.h:144 RoundPay 5; S47Openings.cpp tidlige kit 255; Upgrade :1208 60 + 50 (indeks 1 er gratis);
#  FreeformWorld Price(): relay 25, drive_motor 35, donor 15, fuse 5, film 10, fuel 10, cell 4, spare 15, salt 3, feed 6; pumpe $1 per 1/4
#  enhet, 12 km per enhet; FieldLife.h:43-44 tauing $35-60, 45-90 min.
# Forutsetninger (anslag, ikke målt i spill): 18 km kjøring per dag, ett hvil per natt pa SARO (4,5 t), ekstra inntekt (fotokrav $20,
# kopier hos souvenirmannen, smajobber) i snitt $6/dag fra dag 3, en tauing ($45) dag 11 og 24, tom tank dag 9 og 20.
import sys

DAYS = 30
EARLY_KITS = [(2, 'cot', 20), (3, 'geiger_tube', 25), (3, 'magnetometer', 40), (4, 'spotlight', 35), (5, 'film', 30), (6, 'logger2', 45), (7, 'alarm', 60)]
UPGRADES = [(8, 'upgrade0', 60), (9, 'upgrade2', 50)]
PARTS = [(3, 'relay', 25), (4, 'drive_motor', 35), (5, 'relay', 25), (7, 'donor', 15), (9, 'relay', 25), (11, 'spare', 15)]
LATE_KITS = [(15, 'transfer', 90), (15, 'array_alarm', 110)]
TOWS = {11: 45, 24: 45}
DRY = {9, 20}


def dishes(day):
    # 12 i starten, en gren (+3) omtrent annenhver dag fra dag 4 (FS47ArrayWear), 27 ved dag 14
    return min(27, 12 + 3 * max(0, (day - 2) // 2))


def run(cards, scen, extras=6.0, log=False):
    cash, carried, loan = 120, 0, 0
    wants = sorted(EARLY_KITS + UPGRADES + PARTS + (LATE_KITS if scen['late'] else []), key=lambda w: w[0])
    bought, spent = [], 0
    week_km, week_prints, week_poor = 0, 0, 0
    rows = []
    review_week = 0
    for day in range(1, DAYS + 1):
        # weekly review first thing on the day the week turns (StepWeeklyReview)
        week = (day - 1) // 7
        while review_week < week:
            review_week += 1
            power = scen['power'](day)
            bill = power + 15 + min(15, week_km // 2) + min(10, 2 * week_prints) + min(15, 3 * week_poor) + carried
            paid = min(cash, bill)
            cash -= paid
            carried = min(80, bill - paid)
            if scen.get('float') and cash > scen['float']:
                cash = scen['float']  # Tom-spak: overskudd over kassebeholdningen går tilbake til fylket
            week_km = week_prints = week_poor = 0
        # the day: cards in the evening, the round, extras; running costs
        income = 20 * cards + (5 if day >= 2 else 0) + (extras if day >= 3 else 0)
        if scen.get('earn_pays_carried'):
            pay = min(income, carried)
            carried -= pay
            income -= pay
        cash += income
        fuel = 18 / 12 * 4  # $ at the pump
        cash -= min(cash, fuel)
        cash -= min(cash, 2)  # standby, waived when not there
        cash -= min(cash, 2 + (3 if day % 7 == 0 else 0))  # film pack/cells/salt over the week
        if day in TOWS:
            fee = TOWS[day]
            if cash >= fee: cash -= fee
            else: carried = min(80, carried + fee)
        if day in DRY and scen['dry_fee']:
            fee = 20
            if cash >= fee: cash -= fee
            else: carried = min(80, carried + fee)
        week_km += 18
        week_prints += 3 if day >= 5 else 0
        week_poor += 1 if day % 3 == 0 else 0
        # purchases: a careful player keeps a week's bill ($80) in hand and buys nothing while anything is carried
        for w in list(wants):
            if w[0] <= day and carried == 0 and cash - w[2] >= 80:
                cash -= w[2]; spent += w[2]; bought.append((day, w[1])); wants.remove(w)
        rows.append((day, round(cash), carried, dishes(day), round(cash + spent)))
    return rows, bought, spent


SCEN = {
    'A naa': dict(late=False, dry_fee=False, power=lambda d: 25),
    'B C-pakken': dict(late=True, dry_fee=True, power=lambda d: 25),
    'C B + strom 10+1/antenne': dict(late=True, dry_fee=True, power=lambda d: 10 + dishes(d)),
    'D B + strom 10+2/antenne': dict(late=True, dry_fee=True, power=lambda d: 10 + 2 * dishes(d)),
    'E B + kassebeholdning $250 (Tom)': dict(late=True, dry_fee=True, power=lambda d: 10 + dishes(d), float=250),
    'F B + Earn betaler Carried': dict(late=True, dry_fee=True, power=lambda d: 10 + dishes(d), earn_pays_carried=True),
}

if __name__ == '__main__':
    marks = [8, 15, 22, 29, 30]
    for name, sc in SCEN.items():
        print(f'== {name}')
        for cards in (1, 2, 3):
            rows, bought, spent = run(cards, sc)
            cells = ' '.join(f'd{d}:{c}' + (f'(+{k:.0f} ark)' if k else '') for d, c, k, _, _ in rows if d in marks)
            n3 = rows[21][4] - rows[14][4]; n4 = rows[28][4] - rows[21][4]
            print(f'  {cards} kort: {cells} | netto uten kjop uke3 {n3:+d} uke4 {n4:+d} | kjopt ${spent}, sist {bought[-1] if bought else "-"}')
    if '-v' in sys.argv:
        for cards in (1, 2, 3):
            print('dag/kasse/carried/antenner, 2 kort, scenario C:' if cards == 2 else '', end='')
        rows, _, _ = run(2, SCEN['C B + strom 10+1/antenne'])
        print(rows)

# Cash for every day, every scenario and 1-3 cards: econ_days.csv (dag, scenario, kort, kasse, carried, antenner)
if __name__ == '__main__':
    with open(__file__.replace('econ.py', 'econ_days.csv'), 'w') as out:
        out.write('scenario,kort,dag,kasse,carried,antenner\n')
        for name, sc in SCEN.items():
            for cards in (1, 2, 3):
                for d, c, k, n, _ in run(cards, sc)[0]:
                    out.write(f'{name},{cards},{d},{c},{k:.0f},{n}\n')
