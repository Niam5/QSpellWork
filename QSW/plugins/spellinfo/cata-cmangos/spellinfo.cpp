#include "spellinfo.h"
#include "structure.h"
#include <QBuffer>
#include <QSet>

#include <iostream>
#include <vector>

quint8 m_locale = 0;
EnumHash m_enums;
QStringList m_names;
QObjectList m_metaSpells;
Indexes m_metaSpellIndexes;
Indexes m_internalSpells;
std::vector<QString> m_modifiedStrings;

bool SpellInfo::init()
{
    if (!SkillLine::getDbc().load())
        return false;

    if (!SkillLineAbility::getDbc().load())
        return false;

    if (!SpellDuration::getDbc().load())
        return false;

    if (!SpellCastTimes::getDbc().load())
        return false;

    if (!SpellRadius::getDbc().load())
        return false;

    if (!SpellRange::getDbc().load())
        return false;

    if (!SpellIcon::getDbc().load())
        return false;

    if (!SpellDifficulty::getDbc().load())
        return false;

    if (!SpellAuraOptions::getDbc().load())
        return false;

    if (!SpellAuraRestrictions::getDbc().load())
        return false;

    if (!SpellCastingRequirements::getDbc().load())
        return false;

    if (!SpellCategories::getDbc().load())
        return false;

    if (!SpellClassOptions::getDbc().load())
        return false;

    if (!SpellCooldowns::getDbc().load())
        return false;

    if (!SpellEquippedItems::getDbc().load())
        return false;

    if (!SpellInterrupts::getDbc().load())
        return false;

    if (!SpellLevels::getDbc().load())
        return false;

    if (!SpellPower::getDbc().load())
        return false;

    if (!SpellReagents::getDbc().load())
        return false;

    if (!SpellShapeshift::getDbc().load())
        return false;

    if (!SpellTargetRestrictions::getDbc().load())
        return false;

    if (!SpellTotems::getDbc().load())
        return false;

    if (!SpellEffect::getDbc().load())
        return false;

    if (!AreaGroup::getDbc().load())
        return false;

    if (!AreaTable::getDbc().load())
        return false;

    if (!Spell::getDbc().load())
        return false;

    Spell::fillSpellEffects();

    qDeleteAll(m_metaSpells);
    m_metaSpells.clear();
    QSet<QString> names;
    for (quint32 i = 0; i < Spell::getRecordCount(); ++i)
    {
        if (const Spell::entry* spellInfo = Spell::getRecord(i))
        {
            m_metaSpells << new Spell::meta(spellInfo);
            QString name = spellInfo->name();
            if (names.find(name) == names.end())
                names << name;
        }
    }

    m_names = names.values();

    return true;
}

QStringList SpellInfo::getModifiedSqlDataQueries()
{
    return QStringList
    {
        QStringLiteral("SELECT * FROM spell_template ORDER BY 1")
    };
}

union UnionedValue
{
    quint64 value;
    struct
    {
        quint32 high;
        quint32 low;
    };
};
void SpellInfo::setEnums(EnumHash enums)
{
    m_enums = enums;
}

MPQList SpellInfo::getMPQFiles() const
{
    static MPQList MPQs {
        MPQPair("%locale%/locale-%locale%.MPQ", {
                    "%locale%/wow-update-%locale%-15211.MPQ",
                    "%locale%/wow-update-%locale%-15354.MPQ",
                    "%locale%/wow-update-%locale%-15595.MPQ" })
    };
    return MPQs;
}

QObject* SpellInfo::getMetaSpell(quint32 id, bool realId) const
{
    if (!realId)
        return m_metaSpells.at(id);

    quint32 index = Spell::getDbc().getIndex(id);

    if (index != -1)
        return m_metaSpells.at(index);

    return nullptr;
}

quint32 SpellInfo::getSpellsCount() const
{
    return Spell::getRecordCount();
}

QObjectList SpellInfo::getMetaSpells() const
{
    return m_metaSpells;
}

EnumHash SpellInfo::getEnums() const
{
    return m_enums;
}

quint8 SpellInfo::getLocale() const
{
    return m_locale;
}

QStringList SpellInfo::getNames() const
{
    return m_names;
}

QImage getSpellIcon(quint32 iconId)
{
    const SpellIcon::entry* iconInfo = SpellIcon::getRecord(iconId, true);

    return (iconInfo ? BLP::getBLP(iconInfo->iconPath() + QString(".blp")) : QImage());
}

QString getSpellIconName(quint32 iconId)
{
    const SpellIcon::entry* iconInfo = SpellIcon::getRecord(iconId, true);

    if (!iconInfo)
        return QString();

    return QString(iconInfo->iconPath()).section('\\', -1).toLower();
}

QVariantList getParentSpells(quint32 triggerId)
{
    QVariantList parentSpells;
    for (quint32 i = 0; i < Spell::getRecordCount(); ++i)
    {
        if (const Spell::entry* spellInfo = Spell::getRecord(i))
        {
            QVariantHash parentSpell;
            for (quint8 eff = 0; eff < spellInfo->getEffectCount(); ++eff)
            {
                if (spellInfo->getEffectTriggerSpell(eff) == triggerId)
                {
                    parentSpell["id"] = spellInfo->id;
                    parentSpell["name"] = spellInfo->nameWithRank();
                    parentSpells.append(parentSpell);
                    break;
                }
            }
        }
    }

    return parentSpells;
}

QString splitMask(quint32 mask, Enumerator enumerator)
{
    QString str("");

    EnumIterator itr(enumerator);
    while (itr.hasNext())
    {
        itr.next();
        if (mask & itr.key())
        {
            QString tstr(QString("%0, ").arg(itr.value()));
            str += tstr;
        }
    }
    if (!str.isEmpty())
        str.chop(2);
    return str;
}

QVariantList splitMaskToList(quint32 mask, Enumerator enumerator)
{
    QVariantList flags;
    EnumIterator itr(enumerator);
    while (itr.hasNext())
    {
        itr.next();
        if (mask & itr.key())
        {
            QVariantHash flag;
            flag["name"] = itr.value();
            flag["value"] = QString("0x" + QString("%0").arg(itr.key(), 8, 16, QChar('0')).toUpper());
            flags.append(flag);
        }
    }

    return flags;
}

void RegExpU(const Spell::entry* spellInfo, QRegularExpressionMatch match, QString &str)
{
    if (!match.captured(4).isEmpty())
    {
        if (const Spell::entry* tSpell = Spell::getRecord(match.captured(4).toInt(), true))
        {
            str.replace(match.captured(0), QString("%0")
                .arg(tSpell->getStackAmount()));
        }
    }
    else
    {
        str.replace(match.captured(0), QString("%0")
            .arg(spellInfo->getStackAmount()));
    }
}

void RegExpH(const Spell::entry* spellInfo, QRegularExpressionMatch match, QString &str)
{
    if (!match.captured(4).isEmpty())
    {
        if (const Spell::entry* tSpell = Spell::getRecord(match.captured(4).toInt(), true))
        {
            str.replace(match.captured(0), QString("%0")
                .arg(tSpell->getProcChance()));
        }
    }
    else
    {
        str.replace(match.captured(0), QString("%0")
            .arg(spellInfo->getProcChance()));
    }
}

void RegExpV(const Spell::entry* spellInfo, QRegularExpressionMatch match, QString &str)
{
    if (!match.captured(4).isEmpty())
    {
        if (const Spell::entry* tSpell = Spell::getRecord(match.captured(4).toInt(), true))
        {
            str.replace(match.captured(0), QString("%0")
                .arg(tSpell->getMaxTargetLevel()));
        }
    }
    else
    {
        str.replace(match.captured(0), QString("%0")
            .arg(spellInfo->getMaxTargetLevel()));
    }
}

void RegExpQ(const Spell::entry* spellInfo, QRegularExpressionMatch match, QString &str)
{
    if (!match.captured(3).isEmpty())
    {
        if (!match.captured(4).isEmpty())
        {
            if (const Spell::entry* tSpell = Spell::getRecord(match.captured(4).toInt(), true))
            {
                if (match.captured(2) == QString("/"))
                {
                    str.replace(match.captured(0), QString("%0")
                        .arg(abs(qint32(tSpell->getEffectMiscValueA(match.captured(6).toInt()-1) / match.captured(3).toInt()))));
                }
                else if (match.captured(2) == QString("*"))
                {
                    str.replace(match.captured(0), QString("%0")
                        .arg(abs(qint32(tSpell->getEffectMiscValueA(match.captured(6).toInt()-1) * match.captured(3).toInt()))));
                }
            }
        }
        else
        {
            if (match.captured(2) == QString("/"))
            {
                str.replace(match.captured(0), QString("%0")
                    .arg(abs(qint32(spellInfo->getEffectMiscValueA(match.captured(6).toInt()-1) / match.captured(3).toInt()))));
            }
            else if (match.captured(2) == QString("*"))
            {
                str.replace(match.captured(0), QString("%0")
                    .arg(abs(qint32(spellInfo->getEffectMiscValueA(match.captured(6).toInt()-1) * match.captured(3).toInt()))));
            }
        }
    }
    else if (!match.captured(4).isEmpty())
    {
        if (const Spell::entry* tSpell = Spell::getRecord(match.captured(4).toInt(), true))
        {
            str.replace(match.captured(0), QString("%0")
                .arg(abs(tSpell->getEffectMiscValueA(match.captured(6).toInt()-1))));
        }
    }
    else
    {
        str.replace(match.captured(0), QString("%0")
            .arg(abs(spellInfo->getEffectMiscValueA(match.captured(6).toInt()-1))));
    }
}

void RegExpI(const Spell::entry* spellInfo, QRegularExpressionMatch match, QString &str)
{
    if (!match.captured(4).isEmpty())
    {
        if (const Spell::entry* tSpell = Spell::getRecord(match.captured(4).toInt(), true))
        {
            if (tSpell->getMaxAffectedTargets() != 0)
            {
                str.replace(match.captured(0), QString("%0")
                    .arg(tSpell->getMaxAffectedTargets()));
            }
            else
            {
                str.replace(match.captured(0), QString("nearby"));
            }
        }
    }
    else
    {
        if (spellInfo->getMaxAffectedTargets() != 0)
        {
            str.replace(match.captured(0), QString("%0")
                .arg(spellInfo->getMaxAffectedTargets()));
        }
        else
        {
            str.replace(match.captured(0), QString("nearby"));
        }
    }
}

void RegExpB(const Spell::entry* spellInfo, QRegularExpressionMatch match, QString &str)
{
    if (!match.captured(3).isEmpty())
    {
        if (!match.captured(4).isEmpty())
        {

            if (const Spell::entry* tSpell = Spell::getRecord(match.captured(4).toInt(), true))
            {
                if (match.captured(2) == QString("/"))
                {
                    str.replace(match.captured(0), QString("%0")
                        .arg(abs(qint32(tSpell->getEffectPointsPerComboPoint(match.captured(6).toInt()-1) / match.captured(3).toInt()))));
                }
                else if (match.captured(2) == QString("*"))
                {
                    str.replace(match.captured(0), QString("%0")
                        .arg(abs(qint32(tSpell->getEffectPointsPerComboPoint(match.captured(6).toInt()-1) * match.captured(3).toInt()))));
                }
            }
        }
        else
        {
            if (match.captured(2) == QString("/"))
            {
                str.replace(match.captured(0), QString("%0")
                    .arg(abs(qint32(spellInfo->getEffectPointsPerComboPoint(match.captured(6).toInt()-1) / match.captured(3).toInt()))));
            }
            else if (match.captured(2) == QString("*"))
            {
                str.replace(match.captured(0), QString("%0")
                    .arg(abs(qint32(spellInfo->getEffectPointsPerComboPoint(match.captured(6).toInt()-1) * match.captured(3).toInt()))));
            }
        }
    }
    else if (!match.captured(4).isEmpty())
    {
        if (const Spell::entry* tSpell = Spell::getRecord(match.captured(4).toInt(), true))
        {
            str.replace(match.captured(0), QString("%0")
                .arg(abs(qint32(tSpell->getEffectPointsPerComboPoint(match.captured(6).toInt()-1)))));
        }
    }
    else
    {
        str.replace(match.captured(0), QString("%0")
            .arg(abs(qint32(spellInfo->getEffectPointsPerComboPoint(match.captured(6).toInt()-1)))));
    }
}

void RegExpA(const Spell::entry* spellInfo, QRegularExpressionMatch match, QString &str)
{
    if (!match.captured(3).isEmpty())
    {
        if (!match.captured(4).isEmpty())
        {
            if (const Spell::entry * tSpell = Spell::getRecord(match.captured(4).toInt(), true))
            {
                if (match.captured(2) == QString("/"))
                {
                    str.replace(match.captured(0), QString("%0")
                        .arg(quint32(tSpell->getRadius(match.captured(6).toInt()-1) / match.captured(2).toInt())));
                }
                else if (match.captured(2) == QString("*"))
                {
                    str.replace(match.captured(0), QString("%0")
                        .arg(quint32(tSpell->getRadius(match.captured(6).toInt()-1) * match.captured(2).toInt())));
                }
            }
        }
        else
        {
            if (match.captured(2) == QString("/"))
            {
                str.replace(match.captured(0), QString("%0")
                    .arg(quint32(spellInfo->getRadius(match.captured(6).toInt()-1) / match.captured(2).toInt())));
            }
            else if (match.captured(2) == QString("*"))
            {
                str.replace(match.captured(0), QString("%0")
                    .arg(quint32(spellInfo->getRadius(match.captured(6).toInt()-1) * match.captured(2).toInt())));
            }
        }
    }
    else if (!match.captured(4).isEmpty())
    {
        if (const Spell::entry* tSpell = Spell::getRecord(match.captured(4).toInt(), true))
        {
            str.replace(match.captured(0), QString("%0")
                .arg(tSpell->getRadius(match.captured(6).toInt()-1)));
        }
    }
    else
    {
        str.replace(match.captured(0), QString("%0")
            .arg(spellInfo->getRadius(match.captured(6).toInt()-1)));
    }
}

void RegExpD(const Spell::entry* spellInfo, QRegularExpressionMatch match, QString &str)
{
    if (!match.captured(3).isEmpty())
    {
        if (!match.captured(4).isEmpty())
        {
            if (const Spell::entry* tSpell = Spell::getRecord(match.captured(4).toInt(), true))
            {
                if (match.captured(2) == QString("/"))
                {
                    str.replace(match.captured(0), QString("%0")
                        .arg(quint32(tSpell->getDuration() / match.captured(3).toInt())));
                }
                else if (match.captured(2) == QString("*"))
                {
                    str.replace(match.captured(0), QString("%0")
                        .arg(quint32(tSpell->getDuration() * match.captured(3).toInt())));
                }
            }
        }
        else
        {
            if (match.captured(2) == QString("/"))
            {
                str.replace(match.captured(0), QString("%0")
                    .arg(quint32(spellInfo->getDuration() / match.captured(3).toInt())));
            }
            else if (match.captured(2) == QString("*"))
            {
                str.replace(match.captured(0), QString("%0")
                    .arg(quint32(spellInfo->getDuration() * match.captured(3).toInt())));
            }
        }
    }
    else if (!match.captured(4).isEmpty())
    {
        if (const Spell::entry* tSpell = Spell::getRecord(match.captured(4).toInt(), true))
        {
            str.replace(match.captured(0), QString("%0 seconds")
                .arg(tSpell->getDuration()));
        }
    }
    else
    {
        str.replace(match.captured(0), QString("%0 seconds")
            .arg(spellInfo->getDuration()));
    }
}

void RegExpO(const Spell::entry* spellInfo, QRegularExpressionMatch match, QString &str)
{
    if (!match.captured(3).isEmpty())
    {
        if (!match.captured(4).isEmpty())
        {
            if (const Spell::entry* tSpell = Spell::getRecord(match.captured(4).toInt(), true))
            {
                if (match.captured(2) == QString("/"))
                {
                    str.replace(match.captured(0), QString("%0")
                        .arg(quint32((tSpell->getTicks(match.captured(6).toInt()-1) * (tSpell->getEffectBasePoints(match.captured(6).toInt()-1) + 1)) / match.captured(3).toInt())));
                }
                else if(match.captured(2) == QString("*"))
                {
                    str.replace(match.captured(0), QString("%0")
                        .arg(quint32((tSpell->getTicks(match.captured(6).toInt()-1) * (tSpell->getEffectBasePoints(match.captured(6).toInt()-1) + 1)) * match.captured(3).toInt())));
                }
            }
        }
        else
        {
            if (match.captured(2) == QString("/"))
            {
                str.replace(match.captured(0), QString("%0")
                    .arg(quint32((spellInfo->getTicks(match.captured(6).toInt()-1) * (spellInfo->getEffectBasePoints(match.captured(6).toInt()-1) + 1)) / match.captured(3).toInt())));
            }
            else if (match.captured(2) == QString("*"))
            {
                str.replace(match.captured(0), QString("%0")
                    .arg(quint32((spellInfo->getTicks(match.captured(6).toInt()-1) * (spellInfo->getEffectBasePoints(match.captured(6).toInt()-1) + 1)) * match.captured(3).toInt())));
            }
        }
    }
    else if (!match.captured(4).isEmpty())
    {
        if (const Spell::entry* tSpell = Spell::getRecord(match.captured(4).toInt(), true))
        {
            str.replace(match.captured(0), QString("%0")
                .arg(tSpell->getTicks(match.captured(6).toInt()-1) * (tSpell->getEffectBasePoints(match.captured(6).toInt()-1) + 1)));
        }
    }
    else
    {
        str.replace(match.captured(0), QString("%0")
            .arg(spellInfo->getTicks(match.captured(6).toInt()-1) * (spellInfo->getEffectBasePoints(match.captured(6).toInt()-1) + 1)));
    }
}

void RegExpS(const Spell::entry* spellInfo, QRegularExpressionMatch match, QString &str)
{
    if (!match.captured(3).isEmpty())
    {
        if (!match.captured(4).isEmpty())
        {
            if (const Spell::entry* tSpell = Spell::getRecord(match.captured(4).toInt(), true))
            {
                if (match.captured(2) == QString("/"))
                {
                    str.replace(match.captured(0), QString("%0")
                        .arg(abs(qint32((tSpell->getEffectBasePoints(match.captured(6).toInt()-1) + 1) / match.captured(3).toInt()))));
                }
                else if (match.captured(2) == QString("*"))
                {
                    str.replace(match.captured(0), QString("%0")
                        .arg(abs(qint32((tSpell->getEffectBasePoints(match.captured(6).toInt()-1) + 1) * match.captured(3).toInt()))));
                }
            }
        }
        else
        {
            if (match.captured(2) == QString("/"))
            {
                str.replace(match.captured(0), QString("%0")
                    .arg(abs(qint32((spellInfo->getEffectBasePoints(match.captured(6).toInt()-1) + 1) / match.captured(3).toInt()))));
            }
            else if (match.captured(2) == QString("*"))
            {
                str.replace(match.captured(0), QString("%0")
                    .arg(abs(qint32((spellInfo->getEffectBasePoints(match.captured(6).toInt()-1) + 1) * match.captured(3).toInt()))));
            }
        }
    }
    else if (!match.captured(4).isEmpty())
    {
        if (const Spell::entry* tSpell = Spell::getRecord(match.captured(4).toInt(), true))
        {
            str.replace(match.captured(0), QString("%0")
                .arg(abs(tSpell->getEffectBasePoints(match.captured(6).toInt()-1) + 1)));
        }
    }
    else
    {
        str.replace(match.captured(0), QString("%0")
            .arg(abs(spellInfo->getEffectBasePoints(match.captured(6).toInt()-1) + 1)));
    }
}

void RegExpT(const Spell::entry* spellInfo, QRegularExpressionMatch match, QString &str)
{
    if (!match.captured(3).isEmpty())
    {
        if (!match.captured(4).isEmpty())
        {
            if (const Spell::entry* tSpell = Spell::getRecord(match.captured(4).toInt(), true))
            {
                if (match.captured(2) == QString("/"))
                {
                    str.replace(match.captured(0), QString("%0")
                        .arg(quint32(tSpell->getAmplitude(match.captured(6).toInt()-1) / match.captured(3).toInt())));
                }
                else if (match.captured(2) == QString("*"))
                {
                    str.replace(match.captured(0), QString("%0")
                        .arg(quint32(tSpell->getAmplitude(match.captured(6).toInt()-1) * match.captured(3).toInt())));
                }
            }
        }
        else
        {
            if (match.captured(2) == QString("/"))
            {
                str.replace(match.captured(0), QString("%0")
                    .arg(quint32(spellInfo->getAmplitude(match.captured(6).toInt()-1) / match.captured(3).toInt())));
            }
            else if (match.captured(2) == QString("*"))
            {
                str.replace(match.captured(0), QString("%0")
                    .arg(quint32(spellInfo->getAmplitude(match.captured(6).toInt()-1) * match.captured(3).toInt())));
            }
        }
    }
    else if (!match.captured(4).isEmpty())
    {

        if (const Spell::entry* tSpell = Spell::getRecord(match.captured(4).toInt(), true))
        {
            if (tSpell->getEffectAmplitude(match.captured(6).toInt()-1))
                str.replace(match.captured(0), QString("%0").arg(tSpell->getAmplitude(match.captured(6).toInt()-1)));
            else
                str.replace(match.captured(0), QString("%0").arg(tSpell->getAmplitude()));
        }
    }
    else
    {
        str.replace(match.captured(0), QString("%0")
            .arg(quint32(spellInfo->getEffectAmplitude(match.captured(6).toInt()-1) / 1000)));
    }
}

void RegExpN(const Spell::entry* spellInfo, QRegularExpressionMatch match, QString &str)
{
    if (!match.captured(4).isEmpty())
    {
        if (const Spell::entry* tSpell = Spell::getRecord(match.captured(4).toInt(), true))
        {
            str.replace(match.captured(0), QString("%0")
                .arg(tSpell->getProcCharges()));
        }
    }
    else
    {
        str.replace(match.captured(0), QString("%0")
            .arg(spellInfo->getProcCharges()));
    }
}

void RegExpX(const Spell::entry* spellInfo, QRegularExpressionMatch match, QString &str)
{
    if (!match.captured(4).isEmpty())
    {
        if (const Spell::entry* tSpell = Spell::getRecord(match.captured(4).toInt(), true))
        {
            str.replace(match.captured(0), QString("%0")
                .arg(tSpell->getEffectChainTarget(match.captured(6).toInt()-1)));
        }
    }
    else
    {
        str.replace(match.captured(0), QString("%0")
            .arg(spellInfo->getEffectChainTarget(match.captured(6).toInt()-1)));
    }
}

void RegExpE(const Spell::entry* spellInfo, QRegularExpressionMatch match, QString &str)
{
    if (!match.captured(4).isEmpty())
    {
        if (const Spell::entry* tSpell = Spell::getRecord(match.captured(4).toInt(), true))
        {
            str.replace(match.captured(0), QString("%0")
                .arg(tSpell->getEffectValueMultiplier(match.captured(6).toInt()-1), 0, 'f', 2));
        }
    }
    else
    {
        str.replace(match.captured(0), QString("%0")
            .arg(spellInfo->getEffectValueMultiplier(match.captured(6).toInt()-1), 0, 'f', 2));
    }
}

QString getDescription(QString str, const Spell::entry* spellInfo)
{
    str.replace("$?","$");
    QRegularExpression rx("\\$+(([/,*])?([0-9]*);)?([d+\\;)(\\d*)?([1-9]*)([A-z])([1-3]*)(([A-z, ]*)\\:([A-z, ]*)\\;)?");
    while (str.contains(rx))
    {
        QRegularExpressionMatch match = rx.match(str);
        if (match.hasMatch())
        {
            QChar symbol = match.captured(5)[0].toLower();
            switch (symbol.toLatin1())
            {
                case 'u': RegExpU(spellInfo, match, str); break;
                case 'h': RegExpH(spellInfo, match, str); break;
                case 'v': RegExpV(spellInfo, match, str); break;
                case 'q': RegExpQ(spellInfo, match, str); break;
                case 'i': RegExpI(spellInfo, match, str); break;
                case 'b': RegExpB(spellInfo, match, str); break;
                case 'm':
                case 's':
                    RegExpS(spellInfo, match, str);
                    break;
                case 'a': RegExpA(spellInfo, match, str); break;
                case 'd': RegExpD(spellInfo, match, str); break;
                case 'o': RegExpO(spellInfo, match, str); break;
                case 't': RegExpT(spellInfo, match, str); break;
                case 'n': RegExpN(spellInfo, match, str); break;
                case 'x': RegExpX(spellInfo, match, str); break;
                case 'e': RegExpE(spellInfo, match, str); break;
                case 'l': str.replace(match.captured(0), match.captured(9)); break;
                case 'g': str.replace(match.captured(0), match.captured(8)); break;
                case 'z': str.replace(match.captured(0), QString("[Home]")); break;
                default: return str;
            }
        }
    }
    return str;
}

QVariantHash SpellInfo::getValues(quint32 id) const
{
    QVariantHash values;

    const Spell::entry* spellInfo = Spell::getRecord(id, true);
    if (!spellInfo)
        return values;

    QByteArray iconData;
    QBuffer buffer;
    buffer.setBuffer(&iconData);
    buffer.open(QIODevice::WriteOnly);
    getSpellIcon(spellInfo->spellIconId).save(&buffer, "PNG");

    // Try load from Wowhead
    if (iconData.isEmpty())
    {
        values["icon"] = QString("http://wow.zamimg.com/images/wow/icons/large/%0.jpg").arg(getSpellIconName(spellInfo->spellIconId));
    }
    else
    {
        values["icon"] = QStringLiteral("data:image/png;base64,") + iconData.toBase64().data();
    }

    values["id"] = spellInfo->id;
    values["name"] = spellInfo->name();
    values["rank"] = spellInfo->rank();
    values["nameWithRank"] = spellInfo->nameWithRank();
    values["description"] = spellInfo->description();
    values["descriptionRegExp"] = getDescription(spellInfo->description(), spellInfo);
    values["tooltip"] = spellInfo->toolTip();
    values["tooltipRegExp"] = getDescription(spellInfo->toolTip(), spellInfo);

    QVariantList parentSpells = getParentSpells(spellInfo->id);
    if (!parentSpells.isEmpty())
    {
        values["hasParents"] = !parentSpells.isEmpty();
        values["parentSpells"] = parentSpells;
    }

    values["modalNextSpell"] = spellInfo->getModalNextSpell();
    values["categoryId"] = spellInfo->getCategory();
    values["spellIconId"] = spellInfo->spellIconId;
    values["activeIconId"] = spellInfo->activeIconId;
    values["spellVisual1"] = spellInfo->spellVisual[0];
    values["spellVisual2"] = spellInfo->spellVisual[1];

    values["spellFamilyId"] = spellInfo->getSpellFamilyName();
    values["spellFamilyName"] = getEnums()["SpellFamily"][spellInfo->getSpellFamilyName()];

    values["spellFamilyFlags1"] = QString(QString("%0").arg(spellInfo->getSpellFamilyFlags(0), 8, 16, QChar('0')).toUpper());
    values["spellFamilyFlags2"] = QString(QString("%0").arg(spellInfo->getSpellFamilyFlags(1), 8, 16, QChar('0')).toUpper());
    values["spellFamilyFlags3"] = QString(QString("%0").arg(spellInfo->getSpellFamilyFlags(2), 8, 16, QChar('0')).toUpper());

    values["spellSchoolMask"] = spellInfo->schoolMask;
    values["spellSchoolMaskNames"] = splitMask(spellInfo->schoolMask, getEnums()["SchoolMask"]);

    values["damageClassId"] = spellInfo->getDamageClass();
    values["damageClassName"] = getEnums()["DamageClass"][spellInfo->getDamageClass()];

    values["preventionTypeId"] = spellInfo->getPreventionType();
    values["preventionTypeName"] = getEnums()["PreventionType"][spellInfo->getPreventionType()];

    if (spellInfo->attributes || spellInfo->attributesEx1 || spellInfo->attributesEx2 ||
        spellInfo->attributesEx3 || spellInfo->attributesEx4 || spellInfo->attributesEx5 ||
        spellInfo->attributesEx6 || spellInfo->attributesEx7 || spellInfo->attributesEx8 ||
        spellInfo->attributesEx9 || spellInfo->attributesEx10)
    {
        values["hasAttributes"] = true;

        if (spellInfo->attributes)
        {
            values["attr"] = QString("0x" + QString("%0").arg(spellInfo->attributes, 8, 16, QChar('0')).toUpper());
            values["attrNames"] = splitMaskToList(spellInfo->attributes, getEnums()["Attributes"]);
        }

        if (spellInfo->attributesEx1)
        {
            values["attrEx1"] = QString("0x" + QString("%0").arg(spellInfo->attributesEx1, 8, 16, QChar('0')).toUpper());
            values["attrEx1Names"] = splitMaskToList(spellInfo->attributesEx1, getEnums()["AttributesEx1"]);
        }

        if (spellInfo->attributesEx2)
        {
            values["attrEx2"] = QString("0x" + QString("%0").arg(spellInfo->attributesEx2, 8, 16, QChar('0')).toUpper());
            values["attrEx2Names"] = splitMaskToList(spellInfo->attributesEx2, getEnums()["AttributesEx2"]);
        }

        if (spellInfo->attributesEx3)
        {
            values["attrEx3"] = QString("0x" + QString("%0").arg(spellInfo->attributesEx3, 8, 16, QChar('0')).toUpper());
            values["attrEx3Names"] = splitMaskToList(spellInfo->attributesEx3, getEnums()["AttributesEx3"]);
        }

        if (spellInfo->attributesEx4)
        {
            values["attrEx4"] = QString("0x" + QString("%0").arg(spellInfo->attributesEx4, 8, 16, QChar('0')).toUpper());
            values["attrEx4Names"] = splitMaskToList(spellInfo->attributesEx4, getEnums()["AttributesEx4"]);
        }

        if (spellInfo->attributesEx5)
        {
            values["attrEx5"] = QString("0x" + QString("%0").arg(spellInfo->attributesEx5, 8, 16, QChar('0')).toUpper());
            values["attrEx5Names"] = splitMaskToList(spellInfo->attributesEx5, getEnums()["AttributesEx5"]);
        }

        if (spellInfo->attributesEx6)
        {
            values["attrEx6"] = QString("0x" + QString("%0").arg(spellInfo->attributesEx6, 8, 16, QChar('0')).toUpper());
            values["attrEx6Names"] = splitMaskToList(spellInfo->attributesEx6, getEnums()["AttributesEx6"]);
        }

        if (spellInfo->attributesEx7)
        {
            values["attrEx7"] = QString("0x" + QString("%0").arg(spellInfo->attributesEx7, 8, 16, QChar('0')).toUpper());
            values["attrEx7Names"] = splitMaskToList(spellInfo->attributesEx7, getEnums()["AttributesEx7"]);
        }

        if (spellInfo->attributesEx8)
        {
            values["attrEx8"] = QString("0x" + QString("%0").arg(spellInfo->attributesEx7, 8, 16, QChar('0')).toUpper());
            values["attrEx8Names"] = splitMaskToList(spellInfo->attributesEx8, getEnums()["AttributesEx8"]);
        }

        if (spellInfo->attributesEx9)
        {
            values["attrEx9"] = QString("0x" + QString("%0").arg(spellInfo->attributesEx9, 8, 16, QChar('0')).toUpper());
            values["attrEx9Names"] = splitMaskToList(spellInfo->attributesEx9, getEnums()["AttributesEx9"]);
        }

        if (spellInfo->attributesEx10)
        {
            values["attrEx10"] = QString("0x" + QString("%0").arg(spellInfo->attributesEx10, 8, 16, QChar('0')).toUpper());
            values["attrEx10Names"] = splitMaskToList(spellInfo->attributesEx10, getEnums()["AttributesEx10"]);
        }
    }

    if (spellInfo->getTargets())
    {
        values["targets"] = QString("0x" + QString("%0").arg(spellInfo->getTargets(), 8, 16, QChar('0')).toUpper());
        values["targetsNames"] = splitMaskToList(spellInfo->getTargets(), getEnums()["TargetFlag"]);
    }

    if (spellInfo->getTargetCreatureType())
    {
        values["creatureType"] = QString("0x" + QString("%0").arg(spellInfo->getTargetCreatureType(), 8, 16, QChar('0')).toUpper());
        values["creatureTypeNames"] = splitMaskToList(spellInfo->getTargetCreatureType(), getEnums()["CreatureType"]);
    }

    if (spellInfo->getStances())
    {
        values["stances"] = QString("0x" + QString("%0").arg(spellInfo->getStances(), 16, 16, QChar('0')).toUpper());
        values["stancesNames"] = splitMaskToList(spellInfo->getStances(), getEnums()["ShapeshiftForm"]);
    }

    if (spellInfo->getStancesNot())
    {
        values["stancesNot"] = QString("0x" + QString("%0").arg(spellInfo->getStancesNot(), 16, 16, QChar('0')).toUpper());
        values["stancesNotNames"] = splitMaskToList(spellInfo->getStancesNot(), getEnums()["ShapeshiftForm"]);
    }

    for (quint32 i = 0; i < SkillLineAbility::getRecordCount(); ++i)
    {
        const SkillLineAbility::entry* skillInfo = SkillLineAbility::getRecord(i);
        if (skillInfo->id && skillInfo->spellId == spellInfo->id)
        {
            if (const SkillLine::entry* skill = SkillLine::getRecord(skillInfo->skillId, true))
            {
                values["skillId"] = skill->id;
                values["skillName"] = skill->name();
                values["reqSkillValue"] = skillInfo->requiredSkillValue;
                values["forwardSpellId"] = skillInfo->forwardSpellId;
                values["minSkillValue"] = skillInfo->minValue;
                values["maxSkillValue"] = skillInfo->maxValue;
                values["charPoints1"] = skillInfo->charPoints[0];
                values["charPoints2"] = skillInfo->charPoints[1];
            }
            break;
        }
    }

    values["spellLevel"] = spellInfo->getSpellLevel();
    values["baseLevel"] = spellInfo->getBaseLevel();
    values["maxLevel"] = spellInfo->getMaxLevel();
    values["maxTargetLevel"] = spellInfo->getMaxTargetLevel();

    if (spellInfo->getEquippedItemClass() != -1)
    {
        values["equipItemClass"] = spellInfo->getEquippedItemClass();
        values["equipItemClassName"] = getEnums()["ItemClass"][spellInfo->getEquippedItemClass()];

        if (spellInfo->getEquippedItemSubClassMask())
        {
            values["equipItemSubClassMask"] = QString("0x" + QString("%0").arg(spellInfo->getEquippedItemSubClassMask(), 8, 16, QChar('0')).toUpper());

            switch (spellInfo->getEquippedItemClass())
            {
                case 2: // WEAPON
                    values["equipItemSubClassMaskNames"] = splitMask(spellInfo->getEquippedItemSubClassMask(), getEnums()["ItemSubClassWeapon"]);
                    break;
                case 4: // ARMOR
                    values["equipItemSubClassMaskNames"] = splitMask(spellInfo->getEquippedItemSubClassMask(), getEnums()["ItemSubClassArmor"]);
                    break;
                case 15: // MISC
                    values["equipItemSubClassMaskNames"] = splitMask(spellInfo->getEquippedItemSubClassMask(), getEnums()["ItemSubClassMisc"]);
                    break;
                default: break;
            }
        }

        if (spellInfo->getEquippedItemInventoryTypeMask())
        {
            values["equipItemInvTypeMask"] = QString("0x" + QString("%0").arg(spellInfo->getEquippedItemInventoryTypeMask(), 8, 16, QChar('0')).toUpper());
            values["equipItemInvTypeMaskNames"] = splitMask(spellInfo->getEquippedItemInventoryTypeMask(), getEnums()["InventoryType"]);
        }
    }

    values["categoryId"] = spellInfo->getCategory();
    values["dispelId"] = spellInfo->getDispel();
    values["dispelName"] = getEnums()["DispelType"][spellInfo->getDispel()];
    values["mechanicId"] = spellInfo->getMechanic();
    values["mechanicName"] = getEnums()["Mechanic"][spellInfo->getMechanic()];

    if (const SpellRange::entry* range = SpellRange::getRecord(spellInfo->rangeIndex, true))
    {
        values["rangeId"] = range->id;
        values["rangeName"] = range->name();
        values["minRange"] = range->minRange;
        values["maxRange"] = range->maxRange;
    }

    if (spellInfo->speed)
        values["speed"] = QString("%0").arg(spellInfo->speed, 0, 'f', 2);

    values["stackAmount"] = spellInfo->getStackAmount();

    if (const SpellCastTimes::entry* castInfo = SpellCastTimes::getRecord(spellInfo->castingTimeIndex, true))
    {
        values["castTimeId"] = castInfo->id;
        values["castTimeValue"] = QString("%0").arg(float(castInfo->castTime) / 1000, 0, 'f', 2);
    }

    values["recoveryInfo"] = spellInfo->getRecoveryTime() || spellInfo->getCategoryRecoveryTime() || spellInfo->getStartRecoveryCategory();

    values["recoveryTime"] = spellInfo->getRecoveryTime();
    values["categoryRecoveryTime"] = spellInfo->getCategoryRecoveryTime();

    values["startRecoveryCategory"] = spellInfo->getStartRecoveryCategory();
    values["startRecoveryTime"] = QString("%0").arg(float(spellInfo->getStartRecoveryTime()), 0, 'f', 2);

    if (const SpellDuration::entry* durationInfo = SpellDuration::getRecord(spellInfo->durationIndex, true))
    {
        values["durationId"] = durationInfo->id;
        values["durationBase"] = durationInfo->duration;
        values["durationPerLevel"] = durationInfo->durationPerLevel;
        values["durationMax"] = durationInfo->maxDuration;
    }

    values["costInfo"] = spellInfo->getManaCost() || spellInfo->getManaCostPercentage();
    values["powerTypeId"] = spellInfo->powerType;
    values["powerTypeName"] = getEnums()["Power"][spellInfo->powerType];
    values["manaCost"] = spellInfo->getManaCost();
    values["manaCostPercentage"] = spellInfo->getManaCostPercentage();
    values["manaCostPerLevel"] = spellInfo->getManaCostPerlevel();
    values["manaPerSecond"] = spellInfo->getManaPerSecond();

    if (spellInfo->getInterruptFlags())
    {
        values["interruptFlags"] = QString("0x" + QString("%0").arg(spellInfo->getInterruptFlags(), 8, 16, QChar('0')).toUpper());
        values["interruptFlagsNames"] = splitMaskToList(spellInfo->getInterruptFlags(), getEnums()["SpellInterruptFlags"]);
    }

    if (spellInfo->getAuraInterruptFlags())
    {
        values["auraInterruptFlags"] = QString("0x" + QString("%0").arg(spellInfo->getAuraInterruptFlags(), 8, 16, QChar('0')).toUpper());
        values["auraInterruptFlagsNames"] = splitMaskToList(spellInfo->getAuraInterruptFlags(), getEnums()["SpellAuraInterruptFlags"]);
    }

    if (spellInfo->getChannelInterruptFlags())
    {
        values["channelInterruptFlags"] = QString("0x" + QString("%0").arg(spellInfo->getChannelInterruptFlags(), 8, 16, QChar('0')).toUpper());
        values["channelInterruptFlagsNames"] = splitMaskToList(spellInfo->getChannelInterruptFlags(), getEnums()["SpellChannelInterruptFlags"]);
    }

    if (spellInfo->getCasterAuraState())
    {
        values["casterAuraState"] = spellInfo->getCasterAuraState();
        values["casterAuraStateName"] = getEnums()["AuraState"][spellInfo->getCasterAuraState()];
    }

    if (spellInfo->getTargetAuraState())
    {
        values["targetAuraState"] = spellInfo->getTargetAuraState();
        values["targetAuraStateName"] = getEnums()["AuraState"][spellInfo->getTargetAuraState()];
    }

    if (spellInfo->getCasterAuraStateNot())
    {
        values["casterAuraStateNot"] = spellInfo->getCasterAuraStateNot();
        values["casterAuraStateNotName"] = getEnums()["AuraState"][spellInfo->getCasterAuraStateNot()];
    }

    if (spellInfo->getTargetAuraStateNot())
    {
        values["targetAuraStateNot"] = spellInfo->getTargetAuraStateNot();
        values["targetAuraStateNotName"] = getEnums()["AuraState"][spellInfo->getTargetAuraStateNot()];
    }

    if (spellInfo->getCasterAuraSpell())
    {
        if (const Spell::entry* spell = Spell::getRecord(spellInfo->getCasterAuraSpell(), true)) {
            values["casterAuraSpell"] = spellInfo->getCasterAuraSpell();
            values["casterAuraSpellName"] = spell->nameWithRank();
        }
    }

    if (spellInfo->getTargetAuraSpell())
    {
        if (const Spell::entry* spell = Spell::getRecord(spellInfo->getTargetAuraSpell(), true)) {
            values["targetAuraSpell"] = spellInfo->getTargetAuraSpell();
            values["targetAuraSpellName"] = spell->nameWithRank();
        }
    }

    if (spellInfo->getExcludeCasterAuraSpell())
    {
        if (const Spell::entry* spell = Spell::getRecord(spellInfo->getExcludeCasterAuraSpell(), true)) {
            values["excludeCasterAuraSpell"] = spellInfo->getExcludeCasterAuraSpell();
            values["excludeCasterAuraSpellName"] = spell->nameWithRank();
        }
    }

    if (spellInfo->getExcludeTargetAuraSpell())
    {
        if (const Spell::entry* spell = Spell::getRecord(spellInfo->getExcludeTargetAuraSpell(), true)) {
            values["excludeTargetAuraSpell"] = spellInfo->getExcludeTargetAuraSpell();
            values["excludeTargetAuraSpellName"] = spell->nameWithRank();
        }
    }

    values["reqSpellFocus"] = spellInfo->getRequiresSpellFocus();
    values["facingCasterFlags"] = spellInfo->getFacingCasterFlags();

    values["procChance"] = spellInfo->getProcChance();
    values["procCharges"] = spellInfo->getProcCharges();

    if (spellInfo->getProcFlags())
    {
        values["procFlags"] = QString("0x" + QString("%0").arg(spellInfo->getProcFlags(), 8, 16, QChar('0')).toUpper());
        values["procNames"] = splitMaskToList(spellInfo->getProcFlags(), getEnums()["ProcFlags"]);
    }

    if (spellInfo->getMaxAffectedTargets())
        values["maxAffectedTargets"] = QString("%0").arg(spellInfo->getMaxAffectedTargets());

    if (spellInfo->getAreaGroupId())
    {
        QVariantList areaList;

        quint32 areaId = spellInfo->getAreaGroupId();
        while (areaId > 0) {
            if (const AreaGroup::entry* spellArea = AreaGroup::getRecord(areaId, true)) {
                QVariantHash areaValues;
                if (spellArea->areaId[0] > 0) {
                    if (const AreaTable::entry* areaTableEntry = AreaTable::getRecord(spellArea->areaId[0], true)) {
                        areaValues["areaId0"] = QString("%0 - %1").arg(spellArea->areaId[0]).arg(areaTableEntry->name());
                    }
                }
                if (spellArea->areaId[1] > 0) {
                    if (const AreaTable::entry* areaTableEntry = AreaTable::getRecord(spellArea->areaId[1], true)) {
                        areaValues["areaId1"] = QString("%0 - %1").arg(spellArea->areaId[1]).arg(areaTableEntry->name());
                    }
                }
                if (spellArea->areaId[2] > 0) {
                    if (const AreaTable::entry* areaTableEntry = AreaTable::getRecord(spellArea->areaId[2], true)) {
                        areaValues["areaId2"] = QString("%0 - %1").arg(spellArea->areaId[2]).arg(areaTableEntry->name());
                    }
                }
                if (spellArea->areaId[3] > 0) {
                    if (const AreaTable::entry* areaTableEntry = AreaTable::getRecord(spellArea->areaId[3], true)) {
                        areaValues["areaId3"] = QString("%0 - %1").arg(spellArea->areaId[3]).arg(areaTableEntry->name());
                    }
                }
                if (spellArea->areaId[4] > 0) {
                    if (const AreaTable::entry* areaTableEntry = AreaTable::getRecord(spellArea->areaId[4], true)) {
                        areaValues["areaId4"] = QString("%0 - %1").arg(spellArea->areaId[4]).arg(areaTableEntry->name());
                    }
                }
                if (spellArea->areaId[5] > 0) {
                    if (const AreaTable::entry* areaTableEntry = AreaTable::getRecord(spellArea->areaId[5], true)) {
                        areaValues["areaId5"] = QString("%0 - %1").arg(spellArea->areaId[5]).arg(areaTableEntry->name());
                    }
                }

                areaId = spellArea->nextGroup;
                areaList.append(areaValues);
            }
            else
                areaId = 0; // avoid infinite loop
        }

        values["areas"] = 1;
        values["areaInfo"] = areaList;
    }

    if (const SpellDifficulty::entry* spellDifficultyInfo = SpellDifficulty::getRecord(spellInfo->spellDifficultyId, true))
    {
        if (spellDifficultyInfo->spellId[0])
        {
            values["spellDifficultySpells10Normal"] = QString("%0").arg(spellDifficultyInfo->spellId[0]);
            if (const Spell::entry* triggerSpell = Spell::getRecord(spellDifficultyInfo->spellId[0], true))
                values["spellDifficultySpells10NormalName"] = QString("%0").arg(triggerSpell->nameWithRank());
            else
                values["spellDifficultySpells10NormalName"] = "Missing";
        }
        if (spellDifficultyInfo->spellId[1])
        {
            values["spellDifficultySpells10Heroic"] = QString("%0").arg(spellDifficultyInfo->spellId[1]);
            if (const Spell::entry* triggerSpell = Spell::getRecord(spellDifficultyInfo->spellId[1], true))
                values["spellDifficultySpells10HeroicName"] = QString("%0").arg(triggerSpell->nameWithRank());
            else
                values["spellDifficultySpells10HeroicName"] = "Missing";
        }
        if (spellDifficultyInfo->spellId[2])
        {
            values["spellDifficultySpells25Normal"] = QString("%0").arg(spellDifficultyInfo->spellId[2]);
            if (const Spell::entry* triggerSpell = Spell::getRecord(spellDifficultyInfo->spellId[2], true))
                values["spellDifficultySpells25NormalName"] = QString("%0").arg(triggerSpell->nameWithRank());
            else
                values["spellDifficultySpells25NormalName"] = "Missing";
        }
        if (spellDifficultyInfo->spellId[3])
        {
            values["spellDifficultySpells25Heroic"] = QString("%0").arg(spellDifficultyInfo->spellId[3]);
            if (const Spell::entry* triggerSpell = Spell::getRecord(spellDifficultyInfo->spellId[3], true))
                values["spellDifficultySpells25HeroicName"] = QString("%0").arg(triggerSpell->nameWithRank());
            else
                values["spellDifficultySpells25HeroicName"] = "Missing";
        }
    }

    QVariantList effectList;
    for (quint8 eff = 0; eff < spellInfo->getEffectCount(); ++eff)
    {
        QVariantHash effectValues;
        effectValues["index"] = eff;
        effectValues["id"] = spellInfo->getEffectId(eff);
        effectValues["name"] = getEnums()["SpellEffect"][spellInfo->getEffectId(eff)];

        effectValues["basePoints"] = spellInfo->getEffectBasePoints(eff) + 1;

        if (spellInfo->getEffectRealPointsPerLevel(eff))
            effectValues["perLevelPoints"] = QString("%0").arg(spellInfo->getEffectRealPointsPerLevel(eff), 0, 'f', 2);

        if (spellInfo->getEffectDieSides(eff) > 1)
            effectValues["dieSidesPoints"] = spellInfo->getEffectBasePoints(eff) + 1 + spellInfo->getEffectDieSides(eff);

        if (spellInfo->getEffectPointsPerComboPoint(eff))
            effectValues["perComboPoints"] = QString("%0").arg(spellInfo->getEffectPointsPerComboPoint(eff), 0, 'f', 2);

        if (spellInfo->getEffectDamageMultiplier(eff) != 1.0f)
            effectValues["damageMultiplier"] = QString("%0").arg(spellInfo->getEffectDamageMultiplier(eff), 0, 'f', 2);

        effectValues["targetA"] = spellInfo->getEffectImplicitTargetA(eff);
        effectValues["targetB"] = spellInfo->getEffectImplicitTargetB(eff);
        effectValues["targetNameA"] = getEnums()["Target"][spellInfo->getEffectImplicitTargetA(eff)];
        effectValues["targetNameB"] = getEnums()["Target"][spellInfo->getEffectImplicitTargetB(eff)];

        qint32 miscA = spellInfo->getEffectMiscValueA(eff);
        effectValues["miscValueA"] = miscA;
        effectValues["amplitude"] = spellInfo->getEffectAmplitude(eff);
        effectValues["auraId"] = spellInfo->getEffectApplyAuraName(eff);
        effectValues["auraName"] = getEnums()["SpellAura"][spellInfo->getEffectApplyAuraName(eff)];

        switch (spellInfo->getEffectApplyAuraName(eff))
        {
            case 29:
                effectValues["mods"] = getEnums()["UnitMod"][miscA];
                break;
            case 107:
            case 108:
                effectValues["mods"] = getEnums()["SpellMod"][miscA];
                break;
            default:
                effectValues["mods"] = miscA;
                break;
        }

        effectValues["miscValueB"] = spellInfo->getEffectMiscValueB(eff);

        effectValues["radiusId"] = spellInfo->getEffectRadiusIndex(eff);
        if (const SpellRadius::entry* spellRadius = SpellRadius::getRecord(spellInfo->getEffectRadiusIndex(eff), true))
            effectValues["radiusValue"] = QString("%0").arg(spellRadius->radius, 0, 'f', 2);

        if (const Spell::entry* triggerSpell = Spell::getRecord(spellInfo->getEffectTriggerSpell(eff), true))
        {
            effectValues["triggerId"] = triggerSpell->id;
            effectValues["triggerName"] = triggerSpell->nameWithRank();
            effectValues["triggerDescription"] = triggerSpell->description();
            effectValues["triggerDescriptionRegExp"] = getDescription(triggerSpell->description(), triggerSpell);
            effectValues["triggerToolTip"] = triggerSpell->toolTip();
            effectValues["triggerToolTipRegExp"] = getDescription(triggerSpell->toolTip(), triggerSpell);
            effectValues["triggerProcChance"] = triggerSpell->getProcChance();
            effectValues["triggerProcCharges"] = triggerSpell->getProcCharges();

            if (triggerSpell->getProcFlags())
            {
                effectValues["triggerProcFlags"] = QString("0x" + QString("%0").arg(triggerSpell->getProcFlags(), 8, 16, QChar('0')).toUpper());
                effectValues["triggerProcNames"] = splitMaskToList(triggerSpell->getProcFlags(), getEnums()["ProcFlags"]);
            }
        }

        effectValues["chainTarget"] = spellInfo->getEffectChainTarget(eff);

        effectValues["mechanicId"] = spellInfo->getEffectMechanic(eff);
        effectValues["mechanicName"] = getEnums()["Mechanic"][spellInfo->getEffectMechanic(eff)];

        if (spellInfo->getEffectSpellClassMask(eff, 0) || spellInfo->getEffectSpellClassMask(eff, 1) || spellInfo->getEffectSpellClassMask(eff, 2))
        {
            effectValues["hasClassMask"] = true;
            effectValues["classMask1"] = QString(QString("%0").arg(spellInfo->getEffectSpellClassMask(eff, 0), 8, 16, QChar('0')).toUpper());
            effectValues["classMask2"] = QString(QString("%0").arg(spellInfo->getEffectSpellClassMask(eff, 1), 8, 16, QChar('0')).toUpper());
            effectValues["classMask3"] = QString(QString("%0").arg(spellInfo->getEffectSpellClassMask(eff, 2), 8, 16, QChar('0')).toUpper());

            if (spellInfo->getEffectId(eff) == 6)
            {
                QVariantList affectList;
                for (quint32 i = 0; i < Spell::getRecordCount(); ++i)
                {
                    if (const Spell::entry* t_spellInfo = Spell::getRecord(i))
                    {
                        bool hasSkill = false;
                        if ((t_spellInfo->getSpellFamilyName() == spellInfo->getSpellFamilyName()) &&
                            ((t_spellInfo->getSpellFamilyFlags(0) & spellInfo->getEffectSpellClassMask(eff, 0)) ||
                             (t_spellInfo->getSpellFamilyFlags(1) & spellInfo->getEffectSpellClassMask(eff, 1)) ||
                             (t_spellInfo->getSpellFamilyFlags(2) & spellInfo->getEffectSpellClassMask(eff, 2))))
                        {
                            for (quint32 sk = 0; sk < SkillLineAbility::getRecordCount(); ++sk)
                            {
                                const SkillLineAbility::entry* skillInfo = SkillLineAbility::getRecord(sk);
                                if (skillInfo && skillInfo->spellId == t_spellInfo->id && skillInfo->skillId > 0)
                                {
                                    hasSkill = true;
                                    break;
                                }
                            }

                            QVariantHash affectValues;
                            affectValues["id"] = t_spellInfo->id;
                            affectValues["name"] = t_spellInfo->nameWithRank();
                            affectValues["hasSkill"] = hasSkill;
                            affectList.append(affectValues);
                        }
                    }
                }
                effectValues["affectInfo"] = affectList;
            }
        }
        effectList.append(effectValues);
    }

    values["effect"] = effectList;

    return values;
}
